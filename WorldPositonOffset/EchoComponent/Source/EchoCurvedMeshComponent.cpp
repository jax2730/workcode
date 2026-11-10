///////////////////////////////////////////////////////////////////////////////
//
// @file EchoCurvedMeshComponent.cpp
// @author changziheng
// @date 2023/1/16
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "EchoCurvedMeshComponent.h"
#include "EchoPiecewiseBezierComponent.h"

namespace Echo
{
	///////////////////////////////////////////////////////////////////////////
	//CurvedMeshComponentInfo

	EDITOR_REFLECT_BEGIN(CurvedMeshComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(strMeshPath, "MeshPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(componentBindName, "BindName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	CurvedMeshComponentInfo::CurvedMeshComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{}

	void CurvedMeshComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<CurvedMeshComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CurvedMeshComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<CurvedMeshComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void CurvedMeshComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		CurvedMeshComponentInfo* pInfo = static_cast<CurvedMeshComponentInfo*> (info);
		reflect::TypeResolver<CurvedMeshComponentInfo>::get()->copyTo(this, pInfo);
	}

	void CurvedMeshComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			CurvedMeshComponentInfo* oriInfo = static_cast<CurvedMeshComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<CurvedMeshComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}

	//////////////////////////////////////////////////////
	//CurvedMeshComponent
	DECLAREACTORCOMTYPE_CPP(CurvedMeshComponent)
		CurvedMeshComponent::CurvedMeshComponent(ActorPtr _actor,
			ActorCOMMoveableType movableType /*= ALL_MOVEABLE*/)
		: NodeActorComponent(_actor, movableType)
		, mBezier(nullptr)
		, bBezierReady(false)
		, mMeshSize( 0.0f,0.0f,0.0f )
		, mRoadNormal( 0.0f,1.0f,0.0f )
		, mRoadTangent( 1.0f,0.0f,0.0f )
	{
	}

	CurvedMeshComponent::~CurvedMeshComponent()
	{
		clearCurvedMesh();
		if (bBezierReady && !mBezier.expired())
		{
			mBezier->deleteCurvedComponent(nullptr);
		}

	}

	void CurvedMeshComponent::init(ActorComponentInfo* info)
	{
	}

	void CurvedMeshComponent::connectCom()
	{
		clearCurvedMesh();
		auto info = static_cast<CurvedMeshComponentInfo*>(m_pActComInfo);
		if (!(info->strMeshPath.empty() || info->componentBindName.empty()))
		{
			setMeshData(info->strMeshPath);
			bool bBezierReady = setComponentBindName(info->componentBindName);
			if (mMesh.getPointer() && bBezierReady)
			{
				update();
			}
		}
	}

	void CurvedMeshComponent::visibleStateChanged(bool visibleState)
	{
		if (visibleState)
		{
			update();
		}
		else
		{
			clearCurvedMesh();
		}
	}

	void CurvedMeshComponent::setMeshData(String mesh)
	{
		mMesh = Echo::MeshMgr::instance()->load(Echo::Name(mesh));
	}

	bool CurvedMeshComponent::setComponentBindName(String name)
	{
		auto com = this->getActor()->getComponent(name);
		if (com)
		{
			mBezier = com.getStaticCastWeakPtr<PiecewiseBezierComponent>();
			mBezier->addCurvedComponent(weak_from_this());
			bBezierReady = true;
			return true;
		}
		else
		{
			return false;
		}
	}

	void CurvedMeshComponent::update()
	{
		if (mMesh.getPointer() != nullptr)
		{
			auto& value = mMesh->getAABB();
			mMeshSize.x = value.vMax.x - value.vMin.x;
			mMeshSize.z = value.vMin.x;
			mMeshSize.y = value.vMax.z - value.vMin.z;
		}
		else
		{
			assert(0);
		}

		///////////////////////

		if (mBezier.expired())
		{
			bBezierReady = false;
			clearCurvedMesh();
			return;
		}

		float road_length = mBezier->getSegmentLengths();

		if (road_length <= 1e-5)
		{
			clearCurvedMesh();
			return;
		}

		static bool initStoT = false;
		if (!initStoT)
		{
			initStoT = true;
		}
		else
		{
			mStoT.clear();
		}

		if (!mMeshObjects.empty())
		{
			for (auto object : mMeshObjects)
			{
				detachFromNode(object);
				delete object;
			}
			mMeshObjects.clear();
		}

		mMeshObjects.resize(mMesh->getNumSubMesh());
		for (size_t i = 0; i < mMesh->getNumSubMesh(); i++)
		{
			mMeshObjects[i] = new TrafficObject(Root::instance()->getMainSceneManager()->generateMovableName());
			attachToNode(mMeshObjects[i]);
		}

		for (size_t meshNumber = 0; meshNumber < mMesh->getNumSubMesh(); ++meshNumber)
		{
			mRoadNormal = Vector3(0, 1, 0);
			mRoadTangent = Vector3(1, 0, 0);

			float lsize = 0 /*mBezier->getLengths()*/;
			size_t road_part = 0;
			for (auto value : mBezier->getLengthVec())
			{
				if ((lsize - value) > 1.0e-5f)
				{
					lsize -= value;
					road_part++;
				}
				else break;
			}
			updatePickRoad(meshNumber, road_part);
		}
		UpdateComRootNode(true);
	}

	void CurvedMeshComponent::updatePickRoad(size_t subNum, size_t road_part)
	{
		float mesh_length = mBezier->getSegmentLengths();

		float scale_size = mesh_length / mMeshSize.x;

		float beginlength = 0;

		String VSFileName = "Helper_VS.txt";
		String PSFileName = "Helper_PS.txt";
		String GenMask = "#define ILLUM_BLINN\n#define DIFFUSE_TEX\n";

		const MaterialWrapper& materialWrapper = mMesh->getSubMesh(uint32(subNum))->getMaterialWrapper();

		if (materialWrapper.isV1())
		{
			VSFileName = materialWrapper.getV1()->mVSName;
			PSFileName = materialWrapper.getV1()->mPSName;
			GenMask = materialWrapper.getV1()->mMaterialMacro;
		}
		else if (materialWrapper.isV2())
		{
			VSFileName = materialWrapper.getV2()->getShaderPassName().toString() + "_VS.txt";
			PSFileName = materialWrapper.getV2()->getShaderPassName().toString() + "_PS.txt";

			std::stringstream defSs;
			const Mat::VariantMasks& vms = materialWrapper.getV2()->getMatVariantMasks();
			for (const Name& v : vms)
			{
				if (v.empty() || v.c_str()[0] == '<')
				{
					continue;
				}
				ShaderMacrosItem macrosItem(v.toString());
				defSs << macrosItem.toString();
			}
			GenMask = defSs.str();
		}
		else
		{
			assert(false && "missing material");
		}

		TrafficObject* _object = mMeshObjects[subNum];
		_object->begin(VSFileName, PSFileName, Echo::ECHO_TRIANGLE_LIST);
		{
			Echo::Vector3 point0_normal;
			Echo::Vector3 point0_tangent;
			Echo::Vector3 point0;
			point0_normal = mRoadNormal;
			point0_tangent = mRoadTangent;

			_object->getSection((unsigned int)0)->setCullMode(Echo::ECHO_CULL_BACK);
			_object->getSection((unsigned int)0)->setShaderMacro(GenMask, GenMask);

			_object->getSection((unsigned int)0)->setMaterial(materialWrapper);

			calculateCoiledTPoint(
				subNum, road_part,
				0, beginlength, scale_size, point0_normal,
				point0_tangent, point0, _object
			);

			mRoadNormal = point0_normal;
			mRoadTangent = point0_tangent;
		}
		_object->end();

	}

	void CurvedMeshComponent::calculateCoiledTPoint(size_t subNum
		, size_t road_part, size_t ai
		, float beginlength, float scale_size
		, Echo::Vector3& point0_normal, Echo::Vector3& point0_tangent
		, Echo::Vector3& point0, Echo::TrafficObject* _object)
	{
		auto pSubMesh = mMesh->getSubMesh(uint32(subNum));
		MeshVertexIndexData data;
		pSubMesh->getVertexIndexData(data);
		for (size_t i = 0; i < data.vertices.size(); ++i)
		{
			size_t part_index = road_part;
			Echo::Vector3 road_point = getPoint(data, subNum, i, false)
				- Echo::Vector3(mMeshSize.z, 0.f, 0.f);
			float point_t = calculatePointT(beginlength, scale_size, part_index, road_point);

			auto beTool = mBezier->getBezierTool();
			Echo::Vector3 point = beTool->Interpolate((unsigned int)part_index, point_t);
			Echo::Vector3 tangent = beTool->Tangent((unsigned int)part_index, point_t).normalisedCopy();
			Echo::Vector3 normal = point0_normal;
			Echo::Vector3 paranormal = tangent.crossProduct(normal).normalisedCopy();

			calculateNormal(tangent, normal, paranormal);

			// position
			{
				Echo::Matrix3 tranformMatrix = {
					0, normal.x, paranormal.x,
					0, normal.y, paranormal.y,
					0, normal.z, paranormal.z
				};
				Echo::Vector3 position = tranformMatrix * road_point + point;
				_object->position(position);
			}

			// normal and tangent
			{
				Quaternion q = Vector3(0, 1.f, 0).getRotationTo(normal);
				Vector3 cnormal = q * data.normals[i];
				_object->normal(cnormal);

				if (0 != data.tangents.size())
				{
					Vector4 road_tangent = data.tangents[i];
					Vector3 ctangent = Vector3(road_tangent.x, road_tangent.y, road_tangent.z) * q;

					_object->tangent(Vector4(ctangent.x, ctangent.y, ctangent.z, road_tangent.w));
				}
			}

			// uv
			{
				Vector3 uv3 = Vector3(road_point.x, road_point.z, 1.f);
				if (0 != data.uvs.size())
				{
					Vector2 road_uv = data.uvs[i];
					uv3 = Vector3(road_uv.x, road_uv.y, 1.f);
				}
				else
				{
					Echo::Matrix3 change_uv = {
						1 / mMeshSize.x, 0,	0,
						0,	1 / mMeshSize.y, 1.f / 2,
						0,	0,	1.f
					};
					uv3 = change_uv * uv3;
				}
				_object->textureCoord(uv3.x, uv3.y);
			}

			point0_tangent = tangent;
			point0_normal = normal;
			point0 = point;
		}

		for (size_t i = 0; i < data.indices.size(); ++i)
		{
			_object->index(data.indices[i]);
		}
	}

	float CurvedMeshComponent::calculatePointT(float beginlength
		, float scale_size, size_t& part_index, Echo::Vector3 road_point)
	{
		if (beginlength < 0) return 0.f;
		float point_t = 0.f;
		float ms = beginlength + road_point.x * scale_size;

		ms = calculateBeyondLength(part_index, ms);

		float length = 0.f;
		for (size_t i = 0; i < part_index; ++i)
		{
			float value = mBezier->getPiecewiseLength(i);
			if (value != -1.f)
				length += value;
		}
		float S = length + ms;
		auto beTool = mBezier->getBezierTool();

		if (mStoT[S] == 0.f)
		{
			point_t = beTool->LengthsTot(ms, part_index);
			mStoT[S] = point_t + part_index;
		}
		else
			point_t = mStoT[S] - part_index;

		point_t = point_t > 1.f ? 1.f : point_t;
		point_t = point_t < 0.f ? 0.f : point_t;

		return point_t;
	}

	float CurvedMeshComponent::calculateBeyondLength(size_t& road_part, float beginlength)
	{
		while (beginlength - mBezier->getPiecewiseLength(road_part) > 0)
		{
			if (abs(beginlength - mBezier->getPiecewiseLength(road_part)) < 1e-4)
			{
				beginlength = mBezier->getPiecewiseLength(road_part);
				break;
			}

			beginlength -= mBezier->getPiecewiseLength(road_part);

			road_part += 1;
			if (road_part >= mBezier->getLengthVec().size())
			{
				road_part -= 1;
				beginlength = mBezier->getPiecewiseLength(road_part);
				break;
			}
		}
		return beginlength;
	}


#define ClearError(vector3v)								  \
	vector3v.x = (abs(vector3v.x) < 1e-6 ? 0.f : vector3v.x); \
	vector3v.y = (abs(vector3v.y) < 1e-6 ? 0.f : vector3v.y); \
	vector3v.z = (abs(vector3v.z) < 1e-6 ? 0.f : vector3v.z);

	void CurvedMeshComponent::calculateNormal(Vector3 Tangent, Vector3& Normal, Vector3& ParNormal)
	{
		Echo::DVector3 WorldNormal = DVector3(0.0, 1.0, 0.0);
		Echo::DVector3 PlaneRay = Tangent.crossProduct(WorldNormal);
		Echo::DVector3 PlaneNormal = PlaneRay.crossProduct(Tangent).normalisedCopy();
		PlaneRay = Tangent.crossProduct(PlaneNormal).normalisedCopy();
		Normal = PlaneNormal;
		ParNormal = PlaneRay;
	}

	void CurvedMeshComponent::clearCurvedMesh()
	{
		for (auto obj : mMeshObjects)
		{
			detachFromNode(obj);
			delete obj;
		}
		mMeshObjects.clear();
	}

	void CurvedMeshComponent::createComRootNodeImpl()
	{
		mComRootNode = Root::instance()->getMainSceneManager()->createSceneNode();
	}

	//////////////////////////////////////////////////////
	//CurvedMeshComponentProFunc
	void CurvedMeshComponentProFunc::initProtocolFunction()
	{

	}
	void CurvedMeshComponentProFunc::uninitProtocolFunction()
	{
	}
}
