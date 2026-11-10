#include "EchoComponentStableHeaders.h"
#include "EchoPavementUtil.h"
#include "EchoSubMesh.h"
#include "EchoTextureManager.h"
#include "EchoMeshMgr.h"
#include "EchoLogManager.h"
#include "EchoPavementComponent.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoInstanceMaterialListener.h"
#include "EchoRoadManager.h"
#include "EchoMaterialManager.h"
#include "EchoMeshSerializer.h"
#include "EchoPavementObject.h"
#include "EchoWorldManager.h"
namespace Echo
{
	/*namespace
	{
		Echo::SceneManager* scenemgr()
		{
			return Root::instance()->getActiveSceneManager();
		}
	}*/

	void PavementTool::DefaultSceneNodes(std::vector<std::vector<Echo::SceneNode*>>& value)
	{
		if (!value.empty())
		{
			for (auto& moves : value)
			{
				for (auto& node : moves)
				{
					node->detachAllObject();
					node->getCreator()->removeSceneNode(node);
					delete node;
				}
				moves.clear();
			}
			value.clear();
		}
	}

	Name PavementTool::GetDstPhyMeshName(const Name& srcPhyMesh)
	{
		std::string phyMeshName = srcPhyMesh.toString() + "phy";
		std::replace(phyMeshName.begin(), phyMeshName.end(), '/', '_');
		phyMeshName.insert(0, "collision/mesh/");
		Name dstPhyMesh = Name(phyMeshName);
		
		return dstPhyMesh;
	};

	std::shared_ptr<PavementTool::PavementUtilMesh> PavementTool::FindPlaceUtilMeshData(
		size_t place, std::vector<std::shared_ptr<PavementTool::PavementUtilMesh>>& datas)
	{
		size_t PlaceIndex = 0;
		int bUsePlace = -1;
		for (size_t i = 0; i < datas.size(); ++i)
		{
			if ((bUsePlace == -1 && datas[i]->mbUsedPlace == 0) ||
				((bUsePlace == 0 || bUsePlace == -1) && datas[i]->mbUsedPlace == place + 1))
			{
				PlaceIndex = i;
				bUsePlace = datas[i]->mbUsedPlace;
			}
		}
		if (datas.size() > 0)
			return datas[PlaceIndex];
		else
			return nullptr;
	}

	AxisAlignedBox PavementTool::UpdateAsyncModelPart(size_t pick_index, float pick_length, int UpdateWay
		, std::vector<TrafficObject*>& resultdata, const PavementTool::AsyncModelData& ModelData
		, std::shared_ptr<PavementUtilMesh> MeshData, std::vector<AxisAlignedBox>& mABs
		, const CalcualteNormalData_& p_N
		, std::vector<float>& mABLength, float& mablength, int create_num, SceneManager* sceneManager)
	{
		if(Root::instance()->UsePavementTestNewMode || !MeshData) return AxisAlignedBox();

		int segment = int(pick_length / MeshData->GetOffset().x);
		AxisAlignedBox Box;
		PrivateMeshSharedPtr& only_mesh = MeshData->mMeshData->only_mesh;

		MeshPtr basemesh = only_mesh->mesh;
		std::vector<MeshPtr> LodMesh = basemesh->m_vtLodMesh;
		LodMesh.insert(LodMesh.begin(), basemesh);

		float segment_length = pick_length / segment;
		float scale_size = segment_length / MeshData->GetOffset().x;

		int vSize = segment / MeshData->mMaxCountInPoint + 1;

		size_t lodlevel = LodMesh.size();
		for (size_t lod = 0; lod < lodlevel; ++lod)
		{
			MeshPtr mesh = LodMesh[lod];
			size_t SubSum = mesh->getNumSubMesh();
			float ablength = 0.f;
			for (size_t sub_num = 0; sub_num < SubSum; ++sub_num)
			{
				String VSFileName = "Helper_VS.txt";
				String PSFileName = "Helper_PS.txt";
				String GenMask = "#define ILLUM_BLINN\n#define DIFFUSE_TEX\n";

				const MaterialWrapper& mat = mesh->getSubMesh(Echo::uint32(sub_num))->getMaterialWrapper();
				// TODO: MaterialV2

				if (mat.isV1())
				{
					Material* pMat = mat.getV1();
				
					VSFileName = pMat->mVSName;
					PSFileName = pMat->mPSName;
					GenMask = pMat->mMaterialMacro;
				}

				std::vector<AxisAlignedBox> AABB;
				std::vector<float> ABLength;
				float ab_length = mablength;
				float ab_beginLen = ab_length;
				AxisAlignedBox AB;

				for (size_t k = 0; k < vSize; ++k)
				{
					int number = 0;
					if (segment - MeshData->mMaxCountInPoint > 0)
					{
						number = MeshData->mMaxCountInPoint;
						segment -= MeshData->mMaxCountInPoint;
					}
					else
						number = segment;

					if (create_num >= number)
					{
						create_num -= number;
						continue;
					}

					TrafficObject* _obj = nullptr;
					if ((k/* + vSize * sub_num*/) < resultdata.size())
					{
						_obj = resultdata[k/* + vSize * sub_num*/];
					}
					else
					{
						_obj = new TrafficObject(sceneManager->generateMovableName());
						_obj->delayEnd = true;
						resultdata.push_back(_obj);
					}

					size_t indexcount = only_mesh->SubData[lod][sub_num].road_indices.size();
					size_t pointcount = only_mesh->SubData[lod][sub_num].road_vertexes.size();

					_obj->begin(VSFileName, PSFileName, Echo::ECHO_TRIANGLE_LIST, (EM_LOD_LEVEL)lod);
					_obj->setVertexCount(pointcount * number);
					_obj->setIndexCount(indexcount * number);
					{
						_obj->getSection((unsigned int)sub_num,(EM_LOD_LEVEL)lod)->setCullMode(Echo::ECHO_CULL_BACK);
						_obj->getSection((unsigned int)sub_num, (EM_LOD_LEVEL)lod)->setShaderMacro(GenMask, GenMask);
						_obj->getSection((unsigned int)sub_num, (EM_LOD_LEVEL)lod)->setMaterial(mesh->getSubMesh(Echo::uint32(sub_num))->getMaterialWrapper());
					}

					for (int ai = 0; ai < number; ++ai)
					{
						if (create_num != -1)
						{
							ai = create_num > number - 1 ? number - 1 : create_num;
						}

						AxisAlignedBox result = CalculateAsyncPoint(
							pick_index, MeshData, pick_length, ai, ab_length, scale_size,
							only_mesh->SubData[lod][sub_num], p_N,
							MeshData->mMeshData->Points[lod][sub_num], UpdateWay, ModelData, _obj
						);

						Box.merge(result);

						if (ab_length < 100.f * (ABLength.size() + 1))
							AB.merge(result);
						else
						{
							AABB.push_back(AB);
							ABLength.push_back(ab_beginLen);

							ab_beginLen = ab_length;

							AB.setDefault();
							AB.merge(result);
						}
						ab_length += segment_length;

						for (size_t i = 0; i < indexcount; ++i)
						{
							_obj->index(only_mesh->SubData[lod][sub_num].road_indices[i] + (int)pointcount * ai);
						}

						if (create_num != -1) break;
					}

					//_obj->pre_end();
					//resultdata.push_back(_obj);
				}

				if (ab_beginLen != ab_length)
				{
					AABB.push_back(AB);
					ABLength.push_back(ab_beginLen);
				}

				if (sub_num == 0)
				{
					mABs = AABB;
					mABLength = ABLength;
				}
				else
				{
					for (size_t i = 0; i < AABB.size(); ++i)
					{
						mABs[i].merge(AABB[i]);
					}
				}
			}
			mablength = ablength;
		}

		return Box;
	}

	AxisAlignedBox PavementTool::CalculateAsyncPoint(size_t road_part
		, std::shared_ptr<PavementUtilMesh> mesh_da, float pick_length, size_t ai, float beginlength, float scale_size
		, const SubMeshData_& SubData, const CalcualteNormalData_& p_N
		, const std::vector<Vector3>& Points, int UpdateWay, const PavementTool::AsyncModelData& ModelData
		, TrafficObject* _object)
	{
		CalcualteNormalData_ mpNormalData = p_N;

		AxisAlignedBox ab;

		std::unordered_map<float, float> StoT;
		for (size_t i = 0; i < SubData.road_vertexes.size(); ++i)
		{
			Echo::Vector3 road_point = Points[i] - Echo::Vector3(mesh_da->mCreateOffset, 0.f, 0.f);

			float picklength = beginlength + road_point.x * scale_size;

			float point_t = PavementModelUtil::CalculatePointT(picklength, road_part, StoT, ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou);

			Echo::Vector3 point = PiecewiseBezierUtil::CInterpolate(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_t);
			Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_t).normalisedCopy();

			Echo::Vector3 normal;
			Echo::Vector3 paranormal;

			if (road_part == 0 && ai == 0 && i == 0 &&
				UpdateWay == 0 &&
				mpNormalData.bf_tangent == Vector3(1.f, 0.f, 0.f))
			{
				mpNormalData.bf_tangent = tangent;
				mpNormalData.bf_point = point;
			}

			Vector3 world_n = Vector3::UNIT_Y;
			if (UpdateWay == 2)
				world_n = PiecewiseBezierUtil::CalcualteCenterNormal(picklength, pick_length, mpNormalData.normal1, mpNormalData.normal2);

			PiecewiseBezierUtil::GetNormalInTangent(UpdateWay, point, tangent, normal, paranormal, mpNormalData.bf_point, mpNormalData.bf_tangent, mpNormalData.bf_normal, world_n);

			/*if (UpdateWay == 2)
				PiecewiseBezierUtil::CalculateTangent(tangent, normal, paranormal);
			else
				PiecewiseBezierUtil::CalculateNormal(tangent, normal, paranormal);*/

			normal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)road_part, pick_length, picklength, normal, tangent);
			paranormal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)road_part, pick_length, picklength, paranormal, tangent);

			// position
			// normal and tangent
			{
				Echo::Matrix3 tranformMatrix = {
					tangent.x, normal.x, paranormal.x,
					tangent.y, normal.y, paranormal.y,
					tangent.z, normal.z, paranormal.z
				};
				Echo::Vector3 pos = road_point;
				pos.x = 0.f;
				Echo::Vector3 position = tranformMatrix * pos + point;

				_object->position(position);
				ab.merge(position);


				Vector3 cnormal = tranformMatrix * SubData.road_vertexes[i].normal;

				cnormal = mesh_da->mPointChangeMatrix * cnormal;

				_object->normal(cnormal);

				if (0 != SubData.road_tangent.size())
				{
					Vector4 road_tangent = SubData.road_tangent[i];
					Vector3 ctangent = tranformMatrix * Vector3(road_tangent.x, road_tangent.y, road_tangent.z);

					ctangent = mesh_da->mPointChangeMatrix * ctangent;

					_object->tangent(Vector4(ctangent.x, ctangent.y, ctangent.z, road_tangent.w));
				}
			}

			// uv
			{
				Vector3 uv3 = Vector3(road_point.x, road_point.z, 1.f);
				if (0 != SubData.road_uv.size())
				{
					Vector2 road_uv = SubData.road_uv[i];
					uv3 = Vector3(road_uv.x, road_uv.y, 1.f);
				}
				else
				{
					Echo::Matrix3 change_uv = {
						1 / mesh_da->GetOffset().x, 0,	0,
						0,	1 / mesh_da->GetOffset().z, 1.f / 2,
						0,	0,	1.f
					};
					uv3 = change_uv * uv3;
				}
				_object->textureCoord(uv3.x, uv3.y);
			}
		}
		return ab;
	}

	// 若存在生成一半道路数据，后续补完功能，必须传入的EntityData中存在之前的数据。
	void PavementTool::UpdateAsyncModelStep(size_t road_part, float pick_length, float step, int UpdateWay, 
		std::shared_ptr<PavementUtilMesh> MeshData, bool pickEntity, float segment_length, 
		const PavementTool::AsyncModelData& ModelData, PavementTool::AABoxData_* data, 
		const CalcualteNormalData_& p_N, int create_num, SceneManager* sceneManager)
	{
		CalcualteNormalData_ mpNormalData = p_N;

		if (!MeshData) return;

		MeshPtr& mesh = MeshData->mMeshData->only_mesh->mesh;
		auto& aabb = mesh->getAABB();

		float beginLength = 0.f;

		if (data->AABBs.size() > 0 && create_num != -1)
		{
			beginLength = segment_length;
		}

		float ab_beginlen = beginLength;

		size_t moveSize = size_t(pick_length / step) - 1;
		std::unordered_map<float, float> StoT;

		ObjectInstanceDataVec BatchDatas;
		TrafficBatchObject* BatchObject = new TrafficBatchObject(sceneManager);
		//BatchObject->initWorldSet(ModelData.mWorldScale, ModelData.mWorldOri);
		BatchObject->init(mesh, ModelData.mWorldPosition, ModelData.mWorldScale, ModelData.mWorldOri);
		AxisAlignedBox ab;

		for (size_t node_index = 0; node_index <= moveSize; ++node_index)
		{
			if (create_num != -1)
			{
				node_index = create_num - 1 > moveSize ? moveSize : create_num - 1;
			}

			size_t index = (size_t)road_part;
			float point_base = PavementModelUtil::CalculatePointT(beginLength, index, StoT, ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou);

			Echo::Vector3 point = PiecewiseBezierUtil::CInterpolate(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_base);
			Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_base).normalisedCopy();
			Echo::Vector3 normal;
			Echo::Vector3 paranormal;

			if (road_part == 0 && node_index == 0 &&
				UpdateWay == 0 &&
				mpNormalData.bf_tangent == Vector3(1.f, 0.f, 0.f))
			{
				mpNormalData.bf_tangent = tangent;
				mpNormalData.bf_point = point;
			}

			Vector3 world_n = Vector3::UNIT_Y;
			if (UpdateWay == 2)
				world_n = PiecewiseBezierUtil::CalcualteCenterNormal(beginLength, pick_length, mpNormalData.normal1, mpNormalData.normal2);

			PiecewiseBezierUtil::GetNormalInTangent(UpdateWay, point, tangent, normal, paranormal, mpNormalData.bf_point, mpNormalData.bf_tangent, mpNormalData.bf_normal, world_n);

			normal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)index, pick_length, beginLength, normal, tangent);
			paranormal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)index, pick_length, beginLength, paranormal, tangent);

			Echo::Matrix3 tranformMatrix = {
					tangent.x, normal.x, paranormal.x,
					tangent.y, normal.y, paranormal.y,
					tangent.z, normal.z, paranormal.z
			};

			Vector3 position = point + tranformMatrix * MeshData->mModelChange[0];

			Echo::Quaternion ori(tranformMatrix * MeshData->mPointChangeMatrix);

			ObjectInstanceData objdata;
			objdata.position = position;
			objdata.scale = MeshData->mModelChange[1];
			objdata.orientation = ori;
			BatchDatas.push_back(objdata);

			AxisAlignedBox ab_step;
			ab_step.merge(aabb.mMaximum + position);
			ab_step.merge(aabb.mMinimum + position);

			position = position + ModelData.mWorldPosition;
			position.y += MeshData->mModelLeftDown.y;

			size_t async_size = data->AsyncDatas.size();
			data->AsyncDatas.push_back(new EntityData_());
			data->AsyncDatas[async_size]->point = point;
			data->AsyncDatas[async_size]->position = position;
			data->AsyncDatas[async_size]->otherValue = MeshData->mModelChange[0];
			data->AsyncDatas[async_size]->ori = ori;
			data->AsyncDatas[async_size]->tranformMatrix = tranformMatrix;
			
			data->AABB.merge(ab_step);

			if (beginLength < 100.f * (data->ABLengths.size() + 1))
			{
				ab.merge(ab_step);
			}
			else
			{
				data->AABBs.push_back(ab);
				data->ABLengths.push_back(ab_beginlen);

				ab_beginlen = beginLength;

				ab.setDefault();
				ab.merge(ab_step);
			}

			beginLength += step;

			if (create_num != -1) break;
		}

		if (!ab.isNull())
		{
			data->AABBs.push_back(ab);
			data->ABLengths.push_back(ab_beginlen);
		}

		data->ab_beginLen = beginLength;
		BatchObject->addInstanceData(BatchDatas);
		data->obj = BatchObject;
	}

	void PavementTool::AsyncPhyModel(std::unordered_map<float, float>& StoTdata
		, float NextLens, std::shared_ptr<PavementTool::PavementUtilMesh> MeshData, float part_length, int UpdataWay
		, const PavementTool::AsyncModelData& ModelData, std::pair<size_t, float>& IndexLens
		, std::vector<std::vector<Vector3>>& VertexDatas, std::vector<std::vector<uint16>>& IndexDatas
		, const CalcualteNormalData_& p_N, std::vector<AxisAlignedBox>& oBoxs)
	{
		CalcualteNormalData_ mpNormalData = p_N;

		float Length = IndexLens.second;

		MeshPtr& mesh = MeshData->mMeshData->only_mesh->mesh;

		std::vector<Vector3> vdata;
		std::vector<uint16> idata;
		PavementTool::ReadPhyModeBufferData(vdata, idata, mesh, Vector3::ZERO);
		PavementModelUtil::RecodeVertex(vdata, MeshData->mModelChange[0], MeshData->mModelChange[1], MeshData->mPointChangeMatrix);
		std::vector<Vector3> InVData;
		std::vector<uint16> InIData;
		AxisAlignedBox ab;

		float index_length = part_length;
		int objNum = int(index_length / MeshData->GetOffset().x);

		float segment_length = index_length / objNum;
		float scale_size = segment_length / MeshData->GetOffset().x;

		int indexNum = 0;
		for (float k = 0; k < 100.f && NextLens - Length > 1e-4; k += segment_length)
		{
			for (size_t pos = 0; pos < vdata.size(); ++pos)
			{
				size_t part_index = IndexLens.first;
				Vector3 road_point = vdata[pos] - Echo::Vector3(MeshData->mCreateOffset, 0.f, 0.f);
				
				float beginlength = Length + road_point.x * scale_size;
				float point_t = PavementModelUtil::CalculatePointT(beginlength, part_index, StoTdata, ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou);

				Echo::Vector3 point = PiecewiseBezierUtil::CInterpolate(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_t);
				Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_t).normalisedCopy();

				Echo::Vector3 normal;
				Echo::Vector3 paranormal;

				Vector3 world_n = Vector3::UNIT_Y;
				if (UpdataWay == 2)
					world_n = PiecewiseBezierUtil::CalcualteCenterNormal(beginlength, part_length, mpNormalData.normal1, mpNormalData.normal2);

				PiecewiseBezierUtil::GetNormalInTangent(UpdataWay, point, tangent, normal, paranormal, mpNormalData.bf_point, mpNormalData.bf_tangent, mpNormalData.bf_normal, world_n);
				
				normal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)part_index, part_length, Length, normal, tangent);
				paranormal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)part_index, part_length, Length, paranormal, tangent);

				// position
				{
					Echo::Matrix3 tranformMatrix = {
						0, normal.x, paranormal.x,
						0, normal.y, paranormal.y,
						0, normal.z, paranormal.z
					};
					Echo::Vector3 position = tranformMatrix * road_point + point;
					InVData.push_back(position);
					ab.merge(position);
				}
			}
			for (size_t ins = 0; ins < idata.size(); ++ins)
			{
				InIData.push_back(idata[ins] + (int)vdata.size() * indexNum);
			}
			Length += segment_length;

			if (InVData.size() + vdata.size() > size_t(65535.f * 0.5f))
			{
				VertexDatas.push_back(InVData);
				IndexDatas.push_back(InIData);
				oBoxs.push_back(ab);
				InVData.clear();
				InIData.clear();
				ab.setDefault();
				indexNum = 0;
			}
			else
				indexNum++;
		}

		if (InVData.size() != 0)
		{
			VertexDatas.push_back(InVData);
			IndexDatas.push_back(InIData);
			oBoxs.push_back(ab);
		}
	}

	void PavementTool::AsyncCalculatePhyStep(std::unordered_map<float, float>& StoTdata
		, float step_size, bool pickEntity, float NextLens, std::shared_ptr<PavementTool::PavementUtilMesh> MeshData
		, float part_length, int UpdataWay, const Vector3& offset
		, const PavementTool::AsyncModelData& ModelData, std::pair<size_t, float>& IndexLens
		, std::vector<PavementTool::EntityData_*>& resultData, Name& phyname, bool havePillar
		, const CalcualteNormalData_& p_N)
	{
		CalcualteNormalData_ mpNormalData = p_N;

		float Length = IndexLens.second;

		float IndexLength = part_length;

		int num = int(IndexLength / step_size);
		float real_step = IndexLength / (float)num;

		size_t ObjNum = 0;
		for (float k = 0; k < 100.f && NextLens - Length > 1e-4; k += real_step, ++ObjNum)
		{
			float point_base = PavementModelUtil::CalculatePointT(Length, IndexLens.first, StoTdata, ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou);
			
			Echo::Vector3 point = PiecewiseBezierUtil::CInterpolate(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_base);
			Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(ModelData.from, ModelData.from_des, ModelData.to, ModelData.to_sou, point_base).normalisedCopy();
			Echo::Vector3 normal;
			Echo::Vector3 paranormal;

			Vector3 world_n = Vector3::UNIT_Y;
			if (UpdataWay == 2)
				world_n = PiecewiseBezierUtil::CalcualteCenterNormal(Length, part_length, mpNormalData.normal1, mpNormalData.normal2);

			PiecewiseBezierUtil::GetNormalInTangent(UpdataWay, point, tangent, normal, paranormal, mpNormalData.bf_point, mpNormalData.bf_tangent, mpNormalData.bf_normal, world_n);

			Matrix3 mx = MeshData->mPointChangeMatrix;

			normal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)IndexLens.first, part_length, Length, normal, tangent);
			paranormal = PavementModelUtil::RotateModelNormal(ModelData.from_angle, ModelData.to_angle, (int)IndexLens.first, part_length, Length, paranormal, tangent);

			Echo::Matrix3 tranformMatrix = {
					tangent.x, normal.x, paranormal.x,
					tangent.y, normal.y, paranormal.y,
					tangent.z, normal.z, paranormal.z
			};

			Vector3 position = point + tranformMatrix * MeshData->mModelChange[0];

			Echo::Quaternion ori(tranformMatrix * mx);

			Echo::Vector3 scale = MeshData->mModelChange[1];

			PavementTool::EntityData_* Pointdata = new PavementTool::EntityData_();

			Pointdata->position = position;
			Pointdata->ori = ori;
			Pointdata->otherValue = scale;
			Pointdata->point = point;

			position = position + offset;
			position.y -= MeshData->GetOffset().y;

			//float TerrainHeight = 0.f;
			//if (/*0 != mModelUtil->GetPillarMoveCount()*/havePillar)
			//{
			//	TerrainHeight = PiecewiseTool::calculateMinTerrainLength(position, ModelData.boxoffset, tranformMatrix, pickEntity, ModelData.mComName);
			//}

			//Pointdata->TerrainHeight = TerrainHeight;
			Pointdata->tranformMatrix = tranformMatrix;

			resultData.push_back(Pointdata);

			Length += real_step;
		}
		MeshPtr& mesh = MeshData->mMeshData->only_mesh->mesh;
		PavementPhyUtil::InitPhyMeshName(mesh->getName(), phyname);
	}

	void PavementTool::ReadPhyModeBufferData(std::vector<Vector3>& VertexData, std::vector<uint16>& IndexData, const Echo::MeshPtr& mesh, const Vector3& offset)
	{
		std::multimap<uint32, Name> mapPhyMeshName;
		mesh->getPhyMeshName(mapPhyMeshName);
		if (mapPhyMeshName.size() == 0)
			mapPhyMeshName.insert({0, mesh->getName()});
		int beginIndex = 0;
		for (auto& meshname : mapPhyMeshName)
		{
			Name PhyMeshName = GetDstPhyMeshName(meshname.second);

			MeshPtr phyMesh = MeshMgr::instance()->getByName(PhyMeshName);
			if (phyMesh.isNull() || !phyMesh->isPrepared())
			{
				phyMesh = MeshMgr::instance()->prepare(meshname.second);
				if (phyMesh.isNull())
					phyMesh = mesh;
			}
			
			for (size_t subn = 0; subn < phyMesh->getNumSubMesh(); ++subn)
			{
				SubMesh* sub = phyMesh->getSubMesh((uint32)subn);
				MeshVertexIndexData data;
				sub->getVertexIndexData(data);
				for (auto& v : data.vertices)
					VertexData.push_back(v + offset);
				for (auto& i : data.indices)
					IndexData.push_back(uint16(i + beginIndex));
				beginIndex += (int)data.vertices.size();
				assert(beginIndex < (int)65535);
			}
		}
	}

	//============================================================================================================
	// PavementModelUtil
	PavementModelUtil::PavementModelUtil(PiecewiseBezierUtil* tool, SceneManager* scenemanager)
		:mBezierUtil(tool)
	{
		stringstream ss;
		ss << this;
		mNamePath = ss.str();
		RoadHelper::instance()->setVaild(mNamePath, true, this);
		RoadManager::instance()->ClearUtilKeyDatas(mNamePath);

		assert(scenemanager && "SceneManager is null in PavementModelUtil ");
		mScenemanager = scenemanager;
		mModelNode = mScenemanager->createSceneNode();

		for (int i = 0; i < LOD_LEVEL_NUM; ++i)
		{
			float d = i * 100.f;
			m_fLodDistance[i] = d;
		}
	}

	PavementModelUtil::~PavementModelUtil()
	{
		RemoveAllNode();
		if (!mModelMoveObjs.empty())
		{
			for (auto& moveObjs : mModelMoveObjs)
			{
				for (auto& obj : moveObjs)
				{
					/*if (obj->getSectionSize() != 0)
						obj->getSection((unsigned int)0)->setMaterial(nullptr);*/
					delete obj;
				}
				moveObjs.clear();
			}
			mModelMoveObjs.clear();
		}

		if (!mMeshDatas.empty())
		{
			mMeshDatas.clear();
		}

		if(mModelNode)
			mModelNode->getCreator()->destroySceneNode(mModelNode);

		for(auto& v:mRequests)
			RoadHelper::instance()->abortRequest(v.second);
		mRequests.clear();

		RoadHelper::instance()->setVaild(mNamePath);

		RoadManager::instance()->m_mapUtilKeyData.erase(mNamePath);
	}

	void PavementModelUtil::RemoveAllNode()
	{
		for (auto& BatchObject : mStepBatchObjs)
		{
			//scenemgr()->removeListener(BatchObject);
			if (BatchObject)
			{
				mModelNode->detachObject(BatchObject);
				delete BatchObject;
			}
		}
		mStepBatchObjs.clear();
	}

	void PavementModelUtil::DefaultAllObjects(const Vector3& offset)
	{
		mModelNode->detachAllObject();
		mModelNode->removeAllChildren();

		if (!mModelMoveObjs.empty())
		{
			for (auto& moveObjs : mModelMoveObjs)
			{
				for (auto& obj : moveObjs)
				{
					/*if (obj->getSectionSize() != 0)
						obj->getSection((unsigned int)0)->setMaterial(nullptr);*/
					delete obj;
				}
				moveObjs.clear();
			}
			mModelMoveObjs.clear();
		}

		//PavementTool::DefaultSceneNodes(mSceneMoveNodes);
		for (auto& BatchObject : mStepBatchObjs)
		{
			//scenemgr()->removeListener(BatchObject);
			delete BatchObject;
		}
		mStepBatchObjs.clear();

		//PavementTool::DefaultSceneNodes(mPillarMoves);
#ifdef ECHO_EDITOR
		if (!mPillarCounts.empty())
		{
			mPillarCounts.clear();
		}
#endif // ECHO_EDITOR
		
		mSegmentLength = 0.f;

		mBoxs.clear();
		mAABBs.clear();
		mABLengths.clear();
		mIndexStoT.clear();

		mModelNode->setPosition(offset);
		mModelNode->_update(true);
	}

	std::shared_ptr<PavementTool::PavementUtilMesh> PavementModelUtil::insertMoveObj(size_t index, float IndexLength)
	{
		auto p_mesh_d = GetPrivateMeshData(index);
		if (!p_mesh_d || !p_mesh_d->mMeshData || !p_mesh_d->mMeshData->only_mesh) return nullptr;

		if (index < mModelMoveObjs.size())
		{
			for (auto& obj : mModelMoveObjs[index])
			{
				/*if (obj->getSectionSize() != 0)
					obj->getSection((unsigned int)0)->setMaterial(nullptr);*/
				mModelNode->detachObject(obj);
				delete obj;
			}
			mModelMoveObjs[index].clear();
			mAABBs[index].clear();
			mABLengths[index].clear();
			mBoxs[index].setDefault();
		}

		int objnum = int(IndexLength / p_mesh_d->GetOffset().x);
		int vSize = objnum / p_mesh_d->mMaxCountInPoint + 1;

		//MeshPtr& mesh = p_mesh_d->mMeshData->only_mesh->mesh;

		//vSize = vSize * mesh->getNumSubMesh();
		
		if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
			vSize = 0;

		std::vector<TrafficObject*> insert_v(vSize, nullptr);
		std::vector<AxisAlignedBox> insert_ab;
		std::vector<float> insert_fo;
		for (auto& obj : insert_v)
		{
			obj = new TrafficObject(mScenemanager->generateMovableName());
			obj->setRenderDistance(mCullDistance);
			obj->setLodDistance(LOD_LEVEL_1, m_fLodDistance[LOD_LEVEL_1]);
			obj->setLodDistance(LOD_LEVEL_2, m_fLodDistance[LOD_LEVEL_2]);
			mModelNode->attachObject(obj);
		}

		if (index >= mModelMoveObjs.size())
		{
			AxisAlignedBox ab;
			mModelMoveObjs.push_back(insert_v);
			mAABBs.push_back(insert_ab);
			mABLengths.push_back(insert_fo);
			mBoxs.push_back(ab);
		}
		else
		{
			mModelMoveObjs[index] = insert_v;
			mAABBs[index] = insert_ab;
			mABLengths[index] = insert_fo;
		}
		return p_mesh_d;
	}

	float PavementModelUtil::insertMoveNode(size_t index, float IndexLength)
	{
		if (index < mStepBatchObjs.size())
		{
			if (mStepBatchObjs[index])
			{
				//scenemgr()->removeListener(mStepBatchObjs[index]);
				mModelNode->detachObject(mStepBatchObjs[index]);
				delete mStepBatchObjs[index];
			}
		}

		int num = int(IndexLength / mSpacingStep);
		float real_step = (num == 0 ? 0.f : IndexLength / (float)num);

		std::vector<AxisAlignedBox> insert_ab;
		std::vector<float> insert_fo;

		if (index >= mStepBatchObjs.size())
		{
			AxisAlignedBox ab;
			mAABBs.push_back(insert_ab);
			mABLengths.push_back(insert_fo);
			mBoxs.push_back(ab);
			mStepBatchObjs.push_back(nullptr);
		}
		else
		{
			mStepBatchObjs[index] = nullptr;
			mAABBs[index] = insert_ab;
			mABLengths[index] = insert_fo;
		}

		return real_step;
	}

	void PavementModelUtil::insertMovePillar(size_t index)
	{
		if (index < mPillarCounts.size())
		{
			mPillarCounts[index].clear();
		}

		std::vector<int> insert_vs;

		if (index >= mPillarCounts.size())
		{
			mPillarCounts.push_back(insert_vs);
		}
		else
		{
			mPillarCounts[index] = insert_vs;
		}
	}

	float PavementModelUtil::UpdateIndexObject(size_t pick_index, float index_length)
	{
		float real_step = 0.f;
		if (index_length == -1.f)
			index_length = mBezierUtil->GetPartLength(pick_index);
		if (0.f != mSpacingStep)
		{
			real_step = insertMoveNode(pick_index, index_length);
			if (0 != mMaxPillarSum)
				insertMovePillar(pick_index);
		}
		else
			insertMoveObj(pick_index, index_length);
		return real_step;
	}

	std::pair<float, float> PavementModelUtil::UpdateMoveObject(size_t pick_index, PiecewiseTool::ActionType action)
	{
		//assert((mMeshData->only_mesh) && "No Mesh Update MoveObject");

		PiecewiseTool::PiecewiseBezier place;
		place = mBezierUtil->CalculatePlace(pick_index);

		float pick_length = mBezierUtil->GetPartLength(pick_index);/*CalculateLength(pick_index);*/
		float pick_bf_length = 0.f;
		if (int(pick_index - 1) >= 0)
			pick_bf_length = mBezierUtil->GetPartLength(pick_index - 1);/*CalculateLength(pick_index - 1);*/

		return UpdateMoveObject(
			pick_bf_length, pick_length,
			pick_index, action, place
		);
	}

	std::pair<float, float> PavementModelUtil::UpdateMoveObject(
		float pick_bf_length, float pick_length,
		size_t pick_index, PiecewiseTool::ActionType action, PiecewiseTool::PiecewiseBezier place)
	{
		//assert((mMeshData->only_mesh) && "No Mesh Update MoveObject");

		float IndexLength = pick_length;
		float real_stepa = 0.f;
		float real_stepb = 0.f;
		float bfIndexLength = pick_bf_length;

		mIndexStoT.clear();

		switch (action)
		{
		case PiecewiseTool::CREATE_ACTION:
		case PiecewiseTool::REMOVE_ACTION:
		{
			if ((place & PiecewiseTool::Middle) || (place & PiecewiseTool::End))
			{
				real_stepb = UpdateIndexObject(pick_index - 1, bfIndexLength);
			}
			if ((place & PiecewiseTool::Begin) || (place & PiecewiseTool::Middle))
			{
				real_stepa = UpdateIndexObject(pick_index, IndexLength);
			}
		} break;
		case PiecewiseTool::DELETE_ACTION:
		{
			bool iserase = false;

			if (mModelMoveObjs.size() > pick_index)
			{
				for (auto& obj : mModelMoveObjs[pick_index])
				{
					mModelNode->detachObject(obj);
					delete obj;
				}
				mModelMoveObjs.erase(std::next(mModelMoveObjs.begin(), pick_index));
				iserase = true;

				insertMoveObj(pick_index - 1, bfIndexLength);
			}

			if (mStepBatchObjs.size() > pick_index)
			{
				//scenemgr()->removeListener(mStepBatchObjs[pick_index]);
				if (mStepBatchObjs[pick_index])
				{
					mModelNode->detachObject(mStepBatchObjs[pick_index]);
					delete mStepBatchObjs[pick_index];
				}
				mStepBatchObjs.erase(std::next(mStepBatchObjs.begin(), pick_index));
				iserase = true;

				real_stepb = insertMoveNode(pick_index - 1, bfIndexLength);
				int num = int(IndexLength / mSpacingStep);
				real_stepa = IndexLength / (float)num;
			}

			if (mPillarCounts.size() > pick_index)
			{
				mPillarCounts.erase(std::next(mPillarCounts.begin(), pick_index));
				iserase = true;
				insertMovePillar(pick_index - 1);
			}

			if (iserase)
			{
				mAABBs.erase(std::next(mAABBs.begin(), pick_index));
				mABLengths.erase(std::next(mABLengths.begin(), pick_index));
				mBoxs.erase(std::next(mBoxs.begin(), pick_index));
			}

			if (place & PiecewiseTool::Begin)
			{
				real_stepa = UpdateIndexObject(pick_index, IndexLength);
			}

		}break;
		case PiecewiseTool::INSERT_ACTION:
		{
			if (place & PiecewiseTool::End)
			{
				real_stepb = UpdateIndexObject(pick_index - 1, bfIndexLength);
			}
			else
			{
				bool isDisconnect = false;

				if (mModelMoveObjs.size() >= pick_index)
				{
					auto p_mesh_d = insertMoveObj(pick_index - 1, bfIndexLength);
					if (p_mesh_d == nullptr) break;

					int objnum = int(IndexLength / p_mesh_d->GetOffset().x);
					int vSize = objnum / p_mesh_d->mMaxCountInPoint + 1;
					//vSize *= p_mesh_d->mMeshData->only_mesh->mesh->getNumSubMesh();
					std::vector<TrafficObject*> insert_v(vSize, nullptr);
					for (auto& obj : insert_v)
					{
						obj = new TrafficObject(mScenemanager->generateMovableName());
						obj->setRenderDistance(mCullDistance);
						obj->setLodDistance(LOD_LEVEL_1, m_fLodDistance[LOD_LEVEL_1]);
						obj->setLodDistance(LOD_LEVEL_2, m_fLodDistance[LOD_LEVEL_2]);
						mModelNode->attachObject(obj);
					}

					mModelMoveObjs.insert(std::next(mModelMoveObjs.begin(), pick_index), insert_v);
					isDisconnect = true;
				}

				if (mStepBatchObjs.size() >= pick_index)
				{
					real_stepb = insertMoveNode(pick_index - 1, bfIndexLength);
					int num = int(IndexLength / mSpacingStep);
					real_stepa = IndexLength / (float)num;

					mStepBatchObjs.insert(std::next(mStepBatchObjs.begin(), pick_index), nullptr);
					isDisconnect = true;
				}

				if (mPillarCounts.size() >= pick_index)
				{
					insertMovePillar(pick_index - 1);
					std::vector<int> insert_vs;
					mPillarCounts.insert(std::next(mPillarCounts.begin(), pick_index), insert_vs);
					isDisconnect = true;
				}

				if (isDisconnect)
				{
					std::vector<AxisAlignedBox> abs;
					std::vector<float> ab_lens;
					AxisAlignedBox ab;
					mAABBs.insert(std::next(mAABBs.begin(), pick_index), abs);
					mABLengths.insert(std::next(mABLengths.begin(), pick_index), ab_lens);
					mBoxs.insert(std::next(mBoxs.begin(), pick_index), ab);
				}
			}

		}break;
		}

		return { real_stepa, real_stepb };
	}

	bool PavementModelUtil::UpdatePickModelPart(size_t pick_index, const Vector3& offset, int create_num)
	{
		if (mBezierUtil->GetPointCount() - 1 > pick_index && !Root::instance()->UsePavementTestNewMode)
		{
			mSegmentPick = pick_index;

			if (mUpdateWay == 0)
				mBezierUtil->GetBfData(pick_index, bef, bef_tangent, bef_normal);

			bool result = UpdatePickModelPart(pick_index, mBezierUtil->GetPartLength(pick_index),
				mBezierUtil->GetPoint(pick_index), mBezierUtil->GetDestination(pick_index),
				mBezierUtil->GetPoint(pick_index + 1), mBezierUtil->GetSource(pick_index + 1),
				mPointAngle[pick_index], mPointAngle[pick_index + 1], offset, create_num
			);

			if (mUpdateWay == 0)	// 新增新控制点时候会导致新增的控制点未计算法线而出现问题
				mBezierUtil->SetBfData(pick_index + 1, bef, bef_tangent, bef_normal);

			return result;
		}
		return false;
	}

	bool PavementModelUtil::UpdatePickModelPart(size_t pick_index, float pick_length,
		const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou,
		float from_angle, float to_angle, const Vector3& offset, int create_num)
	{
		std::shared_ptr<PavementTool::PavementUtilMesh> mesh_d = GetPrivateMeshData(pick_index);
		if (Root::instance()->UsePavementTestNewMode || !mesh_d || !mesh_d->mMeshData || !mesh_d->mMeshData->only_mesh) return false;

		int segment = int(pick_length / mesh_d->GetOffset().x);

		bool isfilled = false;

		float segment_length = pick_length / segment;
		float scale_size = segment_length / mesh_d->GetOffset().x;
		
		
		MeshPtr basemesh = mesh_d->mMeshData->only_mesh->mesh;
		std::vector<MeshPtr> LodMesh = basemesh->m_vtLodMesh;
		LodMesh.insert(LodMesh.begin(), basemesh);

		size_t lodlevel = LodMesh.size();
		for (size_t lod = 0; lod < lodlevel; ++lod)
		{
			MeshPtr mesh = LodMesh[lod];
			for (size_t sub_num = 0; sub_num < mesh->getNumSubMesh(); ++sub_num)
			{
				String VSFileName = "Helper_VS.txt";
				String PSFileName = "Helper_PS.txt";
				String GenMask = "#define ILLUM_BLINN\n#define DIFFUSE_TEX\n";
				if (sub_num < mesh->getNumSubMesh())
				{
					// TODO: MaterialV2
					if ( mesh->getSubMesh(Echo::uint32(sub_num))->getMaterialWrapper().isV1())
					{
						Echo::Material* mptr = mesh->getSubMesh(Echo::uint32(sub_num))->getMaterialWrapper().getV1();
						VSFileName = mptr->mVSName;
						PSFileName = mptr->mPSName;
						GenMask = mptr->mMaterialMacro;
					}
				}

				int vSize = segment / mesh_d->mMaxCountInPoint + 1;

				std::vector<AxisAlignedBox> AABB;
				std::vector<float> ABLength;
				float ab_length = 0.f;
				float ab_beginLen = ab_length;
				AxisAlignedBox AB;



				if (mAABBs.size() > pick_index && mAABBs[pick_index].size() > 0 && create_num != -1)
				{
					AABB = mAABBs[pick_index];
					ABLength = mABLengths[pick_index];
					ab_length = mSegmentLength;
					ab_beginLen = ABLength[ABLength.size() - 1];
					//ab_index = mSegmentIndex;
					AB = AABB[AABB.size() - 1];
					AABB.pop_back();
					ABLength.pop_back();
				}

				for (size_t k = 0; k < vSize; ++k)
				{
					int number = 0;
					if (segment - mesh_d->mMaxCountInPoint > 0)
					{
						number = mesh_d->mMaxCountInPoint;
						segment -= mesh_d->mMaxCountInPoint;
					}
					else
						number = segment;

					if (create_num >= number)
					{
						create_num -= number;
						continue;
					}

					auto& _obj = mModelMoveObjs[pick_index][k/* + vSize * sub_num*/];

					size_t indexcount = mesh_d->mMeshData->only_mesh->SubData[lod][sub_num].road_indices.size();
					size_t pointcount = mesh_d->mMeshData->only_mesh->SubData[lod][sub_num].road_vertexes.size();

					_obj->begin(VSFileName, PSFileName, Echo::ECHO_TRIANGLE_LIST, (EM_LOD_LEVEL)lod);
					_obj->setVertexCount(pointcount * number);
					_obj->setIndexCount(indexcount * number);
					{
						_obj->getSection((unsigned int)sub_num, (EM_LOD_LEVEL)lod)->setCullMode(Echo::ECHO_CULL_BACK);
						_obj->getSection((unsigned int)sub_num, (EM_LOD_LEVEL)lod)->setShaderMacro(GenMask, GenMask);
						_obj->getSection((unsigned int)sub_num, (EM_LOD_LEVEL)lod)->setMaterial(mesh->getSubMesh(Echo::uint32(sub_num))->getMaterialWrapper());

						for (int ai = 0; ai < number; ++ai)
						{
							if (create_num != -1)
							{
								ai = create_num > number - 1 ? number - 1 : create_num;
							}

							if (mSpacingStep == 0.f)
							{
								AxisAlignedBox result = CalculateCoiledPoint(
									sub_num, pick_index, (EM_LOD_LEVEL)lod, pick_length, ai, ab_length, scale_size,
									_obj, mesh_d, from, from_des, to, to_sou, offset, from_angle, to_angle
								);

								mBoxs[pick_index].merge(result);

								if (ab_length < mClipLength * (ABLength.size() + 1))
									AB.merge(result);
								else
								{
									AABB.push_back(AB);
									ABLength.push_back(ab_beginLen);

									ab_beginLen = ab_length;
									//ab_index++;

									AB.setDefault();
									AB.merge(result);
								}
								ab_length += segment_length;
							}

							for (size_t i = 0; i < indexcount; ++i)
							{
								_obj->index(mesh_d->mMeshData->only_mesh->SubData[lod][sub_num].road_indices[i] + (int)pointcount * ai);
							}

							if (ai == number - 1 && k == vSize - 1)
								isfilled = true;

							if (create_num != -1) break;
						}
					}
					_obj->pre_end();
					//_obj->end();
				}

				if (ab_beginLen != ab_length)
				{
					AABB.push_back(AB);
					ABLength.push_back(ab_beginLen);
				}

				mSegmentLength = ab_length;
				//mSegmentIndex = ab_index;

				if (sub_num != 0)
				{
					for (size_t i = 0; i < AABB.size(); ++i)
					{
						if (i < mAABBs[pick_index].size())
						{
							mAABBs[pick_index][i].merge(AABB[i]);
						}
						else
							mAABBs[pick_index].push_back(AABB[i]);
					}
				}
				else
				{
					mAABBs[pick_index] = AABB;
					mABLengths[pick_index] = ABLength;
				}
			}
		}


		mModelNode->_update(true);
		return isfilled;
	}

	void PavementModelUtil::UpdateAsync(size_t pick_index, Vector3 offset, float pick_length
		, const std::vector<TrafficObject*>& restdata, const std::vector<AxisAlignedBox>& mABs
		, const std::vector<float>& mABLength, float mablength)
	{
		if (Root::instance()->UsePavementTestNewMode)
			return;
		stringstream ss;
		ss << pick_index << "_" << false;
		if (mRequests.find(ss.str()) == mRequests.end()) 
		{
			for (auto& v : restdata)
				delete v;
			return;
		}

		if (mModelMoveObjs[pick_index].size() != 0)
		{
			for (auto& obj : mModelMoveObjs[pick_index])
			{
				mModelNode->detachObject(obj);
				delete obj;
			}
			mModelMoveObjs[pick_index].clear();
		}

		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = GetPrivateMeshData(pick_index);

		int segment = int(pick_length / MeshData->GetOffset().x);

		int vSize = segment / MeshData->mMaxCountInPoint + 1;

		for (size_t k = 0; k < vSize; ++k)
		{

			int number = 0;
			if (segment - MeshData->mMaxCountInPoint > 0)
			{
				number = MeshData->mMaxCountInPoint;
				segment -= MeshData->mMaxCountInPoint;
			}
			else
				number = segment;

			auto& _obj = restdata[k/* + sub_num * vSize*/];

			_obj->delay_end();

			_obj->setRenderDistance(mCullDistance);
			_obj->setLodDistance(LOD_LEVEL_1, m_fLodDistance[LOD_LEVEL_1]);
			_obj->setLodDistance(LOD_LEVEL_2, m_fLodDistance[LOD_LEVEL_2]);

			mModelMoveObjs[pick_index].push_back(_obj);

			mModelNode->attachObject(_obj);
		}
		
		AxisAlignedBox box;
		for (auto& ab : mABs)
			box.merge(ab);

		mSegmentLength = mablength;
		mBoxs[pick_index] = box;
		mAABBs[pick_index] = mABs;
		mABLengths[pick_index] = mABLength;

		mModelNode->_update(true);

		mRequests.erase(ss.str());
	}


	void PavementModelUtil::BatchRendering(int pick_index, float step, const Vector3& offset, int create_num)
	{
		if (mBezierUtil->GetPointCount() - 1 > pick_index && !Root::instance()->UsePavementTestNewMode)
		{
			if (mUpdateWay == 0)
				mBezierUtil->GetBfData(pick_index, bef, bef_tangent, bef_normal);

			BatchRendering(pick_index, mBezierUtil->GetPartLength(pick_index), step,
				mBezierUtil->GetPoint(pick_index), mBezierUtil->GetDestination(pick_index),
				mBezierUtil->GetPoint(pick_index + 1), mBezierUtil->GetSource(pick_index + 1),
				mPointAngle[pick_index], mPointAngle[pick_index + 1], offset, create_num);
			mSegmentModeNumber = create_num - 1;
			mSegmentPick = pick_index;

			//mBezierUtil->SetBfData(pick_index + 1, bef, bef_tangent, bef_normal);
		}
	}

	void PavementModelUtil::BatchRendering(int pick_index, float pick_length, float step,
		const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou,
		float from_angle, float to_angle, const Vector3& offset, int create_num)
	{
		float beginLength = 0.f;

		std::vector<AxisAlignedBox> AABB;
		std::vector<float> ABLength;
		float ab_beginLen = beginLength;
		AxisAlignedBox AB;
		//size_t ab_index = 1;

		std::shared_ptr<PavementTool::PavementUtilMesh> mesh_d = GetPrivateMeshData(pick_index);
		if (!mesh_d || !mesh_d->mMeshData || !mesh_d->mMeshData->only_mesh || Root::instance()->UsePavementTestNewMode) return ;
		MeshPtr& mesh = mesh_d->mMeshData->only_mesh->mesh;

		if (mAABBs.size() > pick_index && mAABBs[pick_index].size() > 0 && create_num != -1)
		{
			AABB = mAABBs[pick_index];
			ABLength = mABLengths[pick_index];
			beginLength = mSegmentLength;
			ab_beginLen = ABLength[ABLength.size() - 1];
			//ab_index = mSegmentIndex;
			AB = AABB[AABB.size() - 1];
			AABB.pop_back();
			ABLength.pop_back();
		}

		size_t moveSize = int(pick_length / mSpacingStep) - 1;/*mSceneMoveNodes[pick_index].size() - 1*/;
		ObjectInstanceDataVec BatchDatas;
		TrafficBatchObject* BatchObject = new TrafficBatchObject(mScenemanager);
		mStepBatchObjs[pick_index] = BatchObject;
		//mStepBatchObjs[pick_index]->initWorldSet(mWorldScale, mWorldQri);
		mStepBatchObjs[pick_index]->init(mesh, offset, mWorldScale, mWorldQri);

		for (size_t node_index = 0; node_index <= moveSize; ++node_index)
		{
			if (create_num != -1)
			{
				node_index = create_num - 1 > moveSize ? moveSize : create_num - 1;
			}

			size_t index = (size_t)pick_index;
			float point_base = CalculatePointT(beginLength, index, from, from_des, to, to_sou);

			Echo::Vector3 point = PiecewiseBezierUtil::CInterpolate(from, from_des, to, to_sou, point_base);
			Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(from, from_des, to, to_sou, point_base).normalisedCopy();
			Echo::Vector3 normal;
			Echo::Vector3 paranormal;

			if (pick_index == 0 && node_index == 0 &&
				bef_tangent == Vector3(1.f, 0.f, 0.f))
				bef_tangent = tangent;

			Vector3 world_n = Vector3::UNIT_Y;
			if (mUpdateWay == 2)
				world_n = PiecewiseBezierUtil::CalcualteCenterNormal(beginLength, pick_length, mBezierUtil->GetNormal(pick_index), mBezierUtil->GetNormal(pick_index + 1));

			PiecewiseBezierUtil::GetNormalInTangent(mUpdateWay, point, tangent, normal, paranormal, bef, bef_tangent, bef_normal, world_n);

			normal = RotateModelNormal(from_angle, to_angle, (int)index, pick_length, beginLength, normal, tangent);
			paranormal = RotateModelNormal(from_angle, to_angle, (int)index, pick_length, beginLength, paranormal, tangent);

			Echo::Matrix3 tranformMatrix = {
					tangent.x, normal.x, paranormal.x,
					tangent.y, normal.y, paranormal.y,
					tangent.z, normal.z, paranormal.z
			};

			Vector3 position = point + tranformMatrix * mesh_d->mModelChange[0];

			Echo::Quaternion ori(tranformMatrix * mesh_d->mPointChangeMatrix);

			Echo::Vector3 scale = mesh_d->mModelChange[1];


			mStepBatchObjs[pick_index]->mRenderDistance = mCullDistance;
			mStepBatchObjs[pick_index]->m_fLodDistance[LOD_LEVEL_1] = m_fLodDistance[LOD_LEVEL_1];
			mStepBatchObjs[pick_index]->m_fLodDistance[LOD_LEVEL_2] = m_fLodDistance[LOD_LEVEL_2];

			ObjectInstanceData data;
			data.position = position;
			data.scale = scale;
			data.orientation = ori;

			AxisAlignedBox aabb = mesh->getAABB();
			AxisAlignedBox ab_mesh;
			ab_mesh.merge(aabb.mMaximum + position);
			ab_mesh.merge(aabb.mMinimum + position);

			position = position + offset;
			//position.y += mesh_d->mModelLeftDown.y;
			AxisAlignedBox result =
				InsertGround(mesh_d, point, position, scale, tranformMatrix, ori, pick_index);

			if (!result.isNull())
				mBoxs[pick_index].merge(result);
			mBoxs[pick_index].merge(ab_mesh);

			if (beginLength < mClipLength * (ABLength.size() + 1))
			{
				AB.merge(ab_mesh);
				AB.merge(result);
			}
			else
			{
				AABB.push_back(AB);
				ABLength.push_back(ab_beginLen);
				
				ab_beginLen = beginLength;
				//ab_index++;

				AB.setDefault();
				AB.merge(ab_mesh);
				AB.merge(result);
			}

			beginLength += step;
			BatchDatas.push_back(data);

			if (create_num != -1) break;
		}
		if (ab_beginLen != beginLength)
		{
			AABB.push_back(AB);
			ABLength.push_back(ab_beginLen);
		}

		mSegmentLength = beginLength;

		mAABBs[pick_index] = AABB;
		mABLengths[pick_index] = ABLength;

		//scenemgr()->addListener(BatchObject);
		mModelNode->attachObject(mStepBatchObjs[pick_index]);
		mStepBatchObjs[pick_index]->addInstanceData(BatchDatas);
		mStepBatchObjs[pick_index]->mMesh->load();
		mStepBatchObjs[pick_index]->asyncLastInit();

		mModelNode->_update(true);
	}

	void PavementModelUtil::UpdateAsyncStep(PavementTool::AABoxData_* data, size_t pick_index, float step, int create_num)
	{
		if (Root::instance()->UsePavementTestNewMode)
			return;
		stringstream ss;
		ss << pick_index << "_" << true;
		if (mRequests.find(ss.str()) == mRequests.end()) 
		{
			delete data->obj;
			return;
		}
		if (mStepBatchObjs[pick_index])
		{
			//scenemgr()->removeListener(mStepBatchObjs[pick_index]);
			mModelNode->detachObject(mStepBatchObjs[pick_index]);
			delete mStepBatchObjs[pick_index];
		}

		mBoxs[pick_index] = data->AABB;
		mAABBs[pick_index] = data->AABBs;
		mABLengths[pick_index] = data->ABLengths;
		mStepBatchObjs[pick_index] = data->obj;

		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = GetPrivateMeshData(pick_index);

		AxisAlignedBox aabb = MeshData->mMeshData->only_mesh->mesh->getAABB();
		float beginLength = mABLengths[pick_index][0];
		ObjectInstanceDataVec BatchDatas;

		if (0 != mMaxPillarSum)
		for (size_t i = 0, j = 1; i < data->AsyncDatas.size(); ++i)
		{
			AxisAlignedBox result;
			{
				Vector3 position = data->AsyncDatas[i]->position;
				Vector3 point = data->AsyncDatas[i]->point;
				Quaternion ori = data->AsyncDatas[i]->ori;
				Matrix3 tranformMatrix = data->AsyncDatas[i]->tranformMatrix;
				bool flag = false;
				float offsety = MeshData->GetOffset().y;
				int createSum = 0;

				if (mPillarCounts.size() > pick_index &&
					mPillarCounts[pick_index].size() == data->AsyncDatas.size())
				{
					createSum = mPillarCounts[pick_index][i];
					flag = createSum > 0;
				}
				else
				{
					Ray ray(position, Vector3::NEGATIVE_UNIT_Y);
					float TerrainHeight = PiecewiseTool::calculateMinTerrainLength(ray, MeshData->GetOffset(), tranformMatrix, mPickModel);
					float Length = position.y - TerrainHeight;
					flag = Length > 0;
					if (flag)
					{
						createSum = int(Length / offsety) + 1;
						if (createSum > mMaxPillarSum)
							createSum = mMaxPillarSum;
					}
				}

				if (flag)
				{
					for (int cNum = 1; cNum <= createSum; ++cNum)
					{
						ObjectInstanceData pillardata;

						Vector3 offsetPoint = data->AsyncDatas[i]->otherValue;
						offsetPoint.y = offsetPoint.y - offsety * cNum;
						Vector3 offsetposition = point + tranformMatrix * offsetPoint;

						pillardata.position = offsetposition;
						pillardata.scale = MeshData->mModelChange[1];
						pillardata.orientation = ori;

						BatchDatas.push_back(pillardata);

						AxisAlignedBox ab_pillar;
						ab_pillar.merge(aabb.mMaximum + offsetposition);
						ab_pillar.merge(aabb.mMinimum + offsetposition);

						result.merge(ab_pillar);
					}
				}
			}

			if (beginLength < 100.f * j)
			{
				mAABBs[pick_index][j - 1].merge(result);
			}
			else
			{
				j++;
				mAABBs[pick_index][j - 1].merge(result);
			}

			mBoxs[pick_index].merge(result);

			beginLength += step;
		}

		mSegmentLength = data->ab_beginLen;
		if (BatchDatas.size() != 0)
			mStepBatchObjs[pick_index]->addInstanceData(BatchDatas);
		mStepBatchObjs[pick_index]->mMesh->load();
		mStepBatchObjs[pick_index]->asyncLastInit();

		//scenemgr()->addListener(mStepBatchObjs[pick_index]);
		mModelNode->attachObject(mStepBatchObjs[pick_index]);

		mRequests.erase(ss.str());
		mModelNode->_update(true);
	}

	float PavementModelUtil::CalculatePointT(float beginlength, float scale_size, size_t& part_index, const Vector3& road_point)
	{
		if (beginlength < 0) return 0.f;
		beginlength += road_point.x * scale_size;

		beginlength = CalculateBeyondLength(part_index, beginlength);

		return CalculatePointT(beginlength, part_index,
			mBezierUtil->GetPoint(part_index),
			mBezierUtil->GetDestination(part_index),
			mBezierUtil->GetPoint(part_index + 1),
			mBezierUtil->GetSource(part_index + 1)
		);
	}

	float PavementModelUtil::CalculatePointT(float beginlength, float scale_size, std::unordered_map<float, float>& StoTdata, size_t& part_index, const Vector3& road_point)
	{
		if (beginlength < 0) return 0.f;
		beginlength += road_point.x * scale_size;

		beginlength = CalculateBeyondLength(part_index, beginlength);

		return CalculatePointT(beginlength, part_index, StoTdata,
			mBezierUtil->GetPoint(part_index),
			mBezierUtil->GetDestination(part_index),
			mBezierUtil->GetPoint(part_index + 1),
			mBezierUtil->GetSource(part_index + 1)
		);
	}

	float PavementModelUtil::CalculatePointT(float beginlength, size_t& part_index, const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou)
	{
		if (beginlength < 0)
			return 0.f;

		while (mIndexStoT.size() <= part_index)
			mIndexStoT.push_back(std::unordered_map<float, float>());

		float point_t = 0.f;
		if (mIndexStoT[part_index][beginlength] == 0.f)
		{
			point_t = mBezierUtil->LengthsTot(from, from_des, to, to_sou, beginlength);
			mIndexStoT[part_index][beginlength] = point_t + part_index;
			if (point_t > 1.f)
				point_t = 1.f;
		}
		else
			point_t = mIndexStoT[part_index][beginlength] - part_index;

		point_t = point_t > 1.f ? 1.f : point_t;
		point_t = point_t < 0.f ? 0.f : point_t;

		return point_t;
	}

	float PavementModelUtil::CalculatePointT(float beginlength, const size_t& part_index, std::unordered_map<float, float>& StoTdata,
		const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou)
	{
		if (beginlength < 0)
			return 0.f;

		float point_t = 0.f;
		if (StoTdata[beginlength] == 0.f)
		{
			point_t = PiecewiseBezierUtil::LengthsTot(from, from_des, to, to_sou, beginlength);
			StoTdata[beginlength] = point_t + part_index;
			if (point_t > 1.f)
				point_t = 1.f;
		}
		else
			point_t = StoTdata[beginlength] - part_index;

		point_t = point_t > 1.f ? 1.f : point_t;
		point_t = point_t < 0.f ? 0.f : point_t;

		return point_t;
	}

	//------------------------------------------------------------------------------------------------------------

	float PavementModelUtil::CalculateBeyondLength(size_t& road_part, float beginlength)
	{
		while (beginlength - mBezierUtil->GetPartLength(road_part)/*CalculateLength(road_part)*/ > 0)
		{
			if (abs(beginlength - mBezierUtil->GetPartLength(road_part)/*CalculateLength(road_part)*/) < 1e-4)
			{
				beginlength = mBezierUtil->GetPartLength(road_part)/*CalculateLength(road_part)*/;
				break;
			}

			beginlength -= mBezierUtil->GetPartLength(road_part)/*CalculateLength(road_part)*/;

			road_part += 1;
			if (road_part >= mABLengths.size())
			{
				road_part -= 1;
				beginlength = mBezierUtil->GetPartLength(road_part)/*CalculateLength(road_part)*/;
				break;
			}
		}
		return beginlength;
	}

	PiecewiseTool::PiecewiseBezier PavementModelUtil::CalculatePlace(size_t index)
	{
		PiecewiseTool::PiecewiseBezier place;
		if (index == 0)
			place = PiecewiseTool::Begin;
		else if (index == mBoxs.size())
			place = PiecewiseTool::End;
		else
			place = PiecewiseTool::Middle;
		return place;
	}

	Echo::Vector3 PavementModelUtil::RotateModelNormal(int pick_index, float beginLength, const Vector3& normal, const Vector3& rAxis)
	{
		float angle1 = mPointAngle[pick_index];
		float angle2 = 0.f;
		size_t index = size_t(pick_index + 1);
		if (index < mBezierUtil->GetPointCount())
			angle2 = mPointAngle[index];

		return RotateModelNormal(
			angle1, angle2,
			pick_index, mBezierUtil->GetPartLength(pick_index), beginLength, normal, rAxis
		);
	}

	Echo::Vector3 PavementModelUtil::RotateModelNormal(float from_angle, float to_angle, int pick_index, float pick_length, float beginLength, const Vector3& normal, const Vector3& rAxis)
	{
		Quaternion q;
		if (from_angle != 0 || to_angle != 0)
		{
			float length = pick_length;
			float step = (to_angle - from_angle) / length;
			float angle = from_angle + step * beginLength;
			q.FromAngleAxis(Degree(angle), rAxis);
		}
		return q * normal;
	}

	AxisAlignedBox PavementModelUtil::CalculateCoiledPoint(size_t sub_num, size_t road_part, EM_LOD_LEVEL lodlevel, float pick_length, size_t ai, float beginlength, float scale_size,
		Echo::TrafficObject* _object, std::shared_ptr<PavementTool::PavementUtilMesh> mesh,
		const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, const Vector3& offset, float from_angle, float to_angle)
	{
		AxisAlignedBox ab;

		auto& SubData = mesh->mMeshData->only_mesh->SubData[lodlevel][sub_num];

		float picklength = beginlength;
		for (size_t i = 0; i < SubData.road_vertexes.size(); ++i)
		{
			Echo::Vector3 road_point = GetPoint(mesh, sub_num, i, lodlevel) - Echo::Vector3(mesh->mCreateOffset, 0.f, 0.f);

			picklength = beginlength + road_point.x * scale_size;

			float point_t = CalculatePointT(picklength, road_part, from, from_des, to, to_sou);

			Echo::Vector3 point = PiecewiseBezierUtil::CInterpolate(from, from_des, to, to_sou, point_t);
			Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(from, from_des, to, to_sou, point_t).normalisedCopy();

			Echo::Vector3 normal;
			Echo::Vector3 paranormal;

			if (road_part == 0.f && ai == 0 && i == 0 &&
				mUpdateWay == 0 &&
				bef_tangent == Vector3(1.f, 0.f, 0.f))
			{
				bef_tangent = tangent;
				bef = point;
			}

			Vector3 world_n = Vector3::UNIT_Y;
			if (mUpdateWay == 2)
				world_n = PiecewiseBezierUtil::CalcualteCenterNormal(picklength, pick_length, mBezierUtil->GetNormal(road_part), mBezierUtil->GetNormal(road_part + 1));

			PiecewiseBezierUtil::GetNormalInTangent(mUpdateWay, point, tangent, normal, paranormal, bef, bef_tangent, bef_normal, world_n);

			normal = RotateModelNormal(from_angle, to_angle, (int)road_part, pick_length, picklength, normal, tangent);
			paranormal = RotateModelNormal(from_angle, to_angle, (int)road_part, pick_length, picklength, paranormal, tangent);
			
			Echo::Matrix3 tranformMatrix = {
					tangent.x, normal.x, paranormal.x,
					tangent.y, normal.y, paranormal.y,
					tangent.z, normal.z, paranormal.z
			};

			// position
			{
				Echo::Vector3 pos = road_point;
				pos.x = 0.f;
				Echo::Vector3 position = tranformMatrix * pos + point;

				//InsertGround(point, position, Vector3(1.f,1.f,1.f), tranformMatrix, )

				_object->position(position);
				ab.merge(position);
			}

			// normal and tangent
			{
				Vector3 cnormal = tranformMatrix * SubData.road_vertexes[i].normal;
				cnormal = mesh->mPointChangeMatrix * cnormal;
				_object->normal(cnormal);

				if (0 != SubData.road_tangent.size())
				{
					Vector4 road_tangent = SubData.road_tangent[i];
					Vector3 ctangent = tranformMatrix * Vector3(road_tangent.x, road_tangent.y, road_tangent.z);
					ctangent = mesh->mPointChangeMatrix * ctangent;

					_object->tangent(Vector4(ctangent.x, ctangent.y, ctangent.z, road_tangent.w));
				}
			}

			// uv
			{
				Vector3 uv3 = Vector3(road_point.x, road_point.z, 1.f);
				if (0 != SubData.road_uv.size())
				{
					Vector2 road_uv = SubData.road_uv[i];
					uv3 = Vector3(road_uv.x, road_uv.y, 1.f);
				}
				else
				{
					Echo::Matrix3 change_uv = {
						1 / mesh->GetOffset().x, 0,	0,
						0,	1 / mesh->GetOffset().z, 1.f / 2,
						0,	0,	1.f
					};
					uv3 = change_uv * uv3;
				}
				_object->textureCoord(uv3.x, uv3.y);
			}
		}
		return ab;
	}

	AxisAlignedBox PavementModelUtil::InsertGround(std::shared_ptr<PavementTool::PavementUtilMesh> mesh,
		const Vector3& point, const Vector3& position, const Vector3& scale, 
		const Matrix3& tranformMatrix, const Quaternion& ori, int pick_index)
	{
		AxisAlignedBox ab;
		if (0 != mMaxPillarSum)
		{
			Ray ray(position, Vector3::NEGATIVE_UNIT_Y);
			float TerrainHeight = PiecewiseTool::calculateMinTerrainLength(ray, mesh->GetOffset(), tranformMatrix, mPickModel);

			float Length = position.y - TerrainHeight;

			if (Length > 0)
			{
				float offsety = mesh->GetOffset().y;
				int createSum = int(Length / offsety) + 1;
				if (createSum > mMaxPillarSum)
					createSum = mMaxPillarSum;


				mPillarCounts[pick_index].push_back(createSum);
				ObjectInstanceDataVec datavec;

				for (int cNum = 1; cNum <= createSum; ++cNum)
				{
					ObjectInstanceData objdata;

					Vector3 offsetPoint = mesh->mModelChange[0];
					offsetPoint.y = offsetPoint.y - offsety * cNum;
					Vector3 offsetposition = point + tranformMatrix * offsetPoint;
					
					objdata.scale = scale;
					objdata.position = offsetposition;
					objdata.orientation = ori;
					datavec.push_back(objdata);

					auto& aabb = mesh->mMeshData->only_mesh->mesh->getAABB();
					AxisAlignedBox ab_mesh;
					ab_mesh.merge(aabb.mMaximum + offsetposition);
					ab_mesh.merge(aabb.mMinimum + offsetposition);

					ab.merge(ab_mesh);
				}

				mStepBatchObjs[pick_index]->addInstanceData(datavec);
			}
			else
			{
				if(pick_index < mPillarCounts.size())
					mPillarCounts[pick_index].push_back(0);
				else
				{
					std::vector<int> insert_v(1, 0);
					mPillarCounts.push_back(insert_v);
				}
			}
		}
		return ab;
	}
	//------------------------------------------------------------------------------------------------------------

	void PavementModelUtil::InitStepAndSumType(float step, int sum, int way)
	{
		mSpacingStep = step;
		mMaxPillarSum = sum;
		mUpdateWay = way;
	}

	void PavementModelUtil::InitBezierUtil(PiecewiseBezierUtil* tool)
	{
		mBezierUtil = tool;
	}

	void PavementModelUtil::InitIndexPillarHeight(size_t pillar_index, std::vector<int>& counts)
	{
		if (mPillarCounts.size() <= pillar_index) return;
		mPillarCounts[pillar_index].clear();
		for (auto& data : counts)
		{
			mPillarCounts[pillar_index].push_back(data);
		}
	}

	void PavementModelUtil::ClearMeshDatas()
	{
		mMeshDatas.clear();
	}

	void PavementModelUtil::AddChangeValue(const std::vector<Vector3>& changevalue)
	{
		size_t DataSize = mMeshDatas.size();
		std::shared_ptr<PavementTool::PavementUtilMesh> d(new PavementTool::PavementUtilMesh());
		mMeshDatas.push_back(d);
		mMeshDatas[DataSize]->mModelChange = changevalue;

		Quaternion qu;
		qu.FromEuler(changevalue[2]);

		Matrix3 Matrix;
		qu.ToRotationMatrix(Matrix);

		for (size_t i = 0; i < 3; ++i)
			for (size_t j = 0; j < 3; ++j)
				Matrix[i][j] = (abs(Matrix[i][j]) < 1e-5 ? 0 : Matrix[i][j]);

		mMeshDatas[DataSize]->mPointChangeMatrix = Matrix;
	}

	void PavementModelUtil::SetWeightAndPlace(size_t index, float weight, int pickplace)
	{
		mMeshDatas[index]->mbUsedPlace = pickplace;
		mMeshDatas[index]->mWeighted = weight;
	}

	bool PavementModelUtil::InitMeshPtr(size_t index, MeshPtr& value, const PavementTool::LodMeshDataCache& meshdata)
	{
		if (index >= mMeshDatas.size()) return false;

		std::shared_ptr<PavementTool::AsyncData>& MeshData = mMeshDatas[index]->mMeshData;
		std::shared_ptr<PavementTool::PrivateMeshData>& only_mesh = MeshData->only_mesh;
		size_t max_points = 0;

		Name meshname = value->getName();
		//auto iter = m_mapdata.find(meshname);
		std::shared_ptr<PavementTool::PrivateMeshData> it_data;
		bool needSubData = mSpacingStep == 0;
		if (RoadManager::instance()->GetPathMeshData(meshname, it_data, needSubData))
		{
			only_mesh = it_data;
			if (only_mesh->mesh.isNull())
				only_mesh->mesh = value;

			for (PavementTool::SubMeshDataCache& dataCache : only_mesh->SubData)
			{
				for(PavementTool::SubMeshData_& data : dataCache)
				{
					size_t sum = data.road_vertexes.size();
					max_points = max_points > sum ? max_points : sum;
				}
			}
		}
		else
		{
			std::shared_ptr<PavementTool::PrivateMeshData> meshdes(new PavementTool::PrivateMeshData());

			meshdes->mesh = value;
			only_mesh = meshdes;

			if (mSpacingStep == 0.f && meshdata.empty())
			{
				uint32 num = value->getNumSubMesh();
				std::vector<PavementTool::SubMeshData_> tempVec;
				tempVec.reserve(num);
				for (uint32 sub = 0; sub < num; ++sub)
				{
					Echo::SubMesh* submesh = value->getSubMesh(sub);

					MeshVertexIndexData data;
					submesh->getVertexIndexData(data);
					PavementTool::SubMeshData_ subdata;

					for (size_t j = 0; j < data.vertices.size(); ++j)
					{
						PavementTool::Vertex_ value(data.vertices[j], data.normals[j]);

						subdata.road_vertexes.push_back(value);
						subdata.road_uv.push_back(data.uvs[j]);
						subdata.road_tangent.push_back(data.tangents[j]);
					}

					for (size_t j = 0; j < data.indices.size(); ++j)
						subdata.road_indices.push_back(data.indices[j]);

					size_t sum = subdata.road_vertexes.size();
					max_points = max_points > sum ? max_points : sum;

					tempVec.push_back(subdata);
				}
				meshdes->SubData.push_back(tempVec);

				for (size_t i = 0; i < value->m_vtLodMesh.size(); ++i)
				{
					const MeshPtr& lodMesh = value->m_vtLodMesh[i];
					if (lodMesh->isLoaded())
					{
						uint32 lodsubnum = lodMesh->getNumSubMesh();
						std::vector<PavementTool::SubMeshData_> tempVec_;
						tempVec_.reserve(lodsubnum);
						for (uint32 sub = 0; sub < lodsubnum; ++sub)
						{
							Echo::SubMesh* submesh = lodMesh->getSubMesh(sub);

							MeshVertexIndexData data;
							submesh->getVertexIndexData(data);
							PavementTool::SubMeshData_ subdata;

							for (size_t j = 0; j < data.vertices.size(); ++j)
							{
								PavementTool::Vertex_ value(data.vertices[j], data.normals[j]);

								subdata.road_vertexes.push_back(value);
								subdata.road_uv.push_back(data.uvs[j]);
								subdata.road_tangent.push_back(data.tangents[j]);
							}

							for (size_t j = 0; j < data.indices.size(); ++j)
								subdata.road_indices.push_back(data.indices[j]);

							tempVec_.push_back(subdata);
						}
						meshdes->SubData.push_back(tempVec_);
					}
				}
			}
			else if (!meshdata.empty())
			{
				meshdes->SubData = meshdata;

				for (PavementTool::SubMeshDataCache& dataCache : only_mesh->SubData)
				{
					for (PavementTool::SubMeshData_& data : dataCache)
					{
						size_t sum = data.road_vertexes.size();
						max_points = max_points > sum ? max_points : sum;
					}
				}
			}
			meshdes->vertexNumMax = max_points;

			RoadManager::instance()->SetPathMeshData(meshname, meshdes);
		}

		//mMeshData = MeshData;

		if (max_points != 0)
		{
			RecodeModelPoint(index);
			mMeshDatas[index]->mMaxCountInPoint = 65535 / (int)max_points;
			//mMeshDatas[index]->mMaxCountInPoint = 20000 / (int)max_points;
		}

		AxisAlignedBox box = value->getAABB();

		Vector3 moveyz = Vector3(0.f
			, mMeshDatas[index]->mModelChange[0].y
			, mMeshDatas[index]->mModelChange[0].z);

		Matrix4 mat;
		mat.makeTransform(moveyz, mMeshDatas[index]->mModelChange[1], Quaternion());
		box.transformAffine(mat);
		Matrix4 m(mMeshDatas[index]->mPointChangeMatrix);
		box.transformAffine(m);

		mMeshDatas[index]->mCreateOffset = box.vMin.x;

		mMeshDatas[index]->mModelLeftDown = box.vMin;
		mMeshDatas[index]->mModelRightUp = box.vMax;
		
		RoadManager::instance()->m_mapUtilKeyData[mNamePath] = &mMeshDatas;

		return true;
	}

	std::shared_ptr<PavementTool::PavementUtilMesh> PavementModelUtil::GetPrivateMeshData(size_t index)
	{
		return PavementTool::FindPlaceUtilMeshData(index, mMeshDatas);
	}

	bool PavementModelUtil::MeshDataNotNull()
	{
		if (mMeshDatas.empty()) return false;
		for (auto& mesh : mMeshDatas)
			if (mesh->GetOffset() == Vector3::ZERO) return false;
		return true;
	}

	std::pair<float, AxisAlignedBox> PavementModelUtil::GetSegmentABox(size_t pick_index, size_t index, const Vector3& offset)
	{
		assert((pick_index < mAABBs.size()) && (index < mAABBs[pick_index].size()));
		std::pair<float, AxisAlignedBox> result;
		result.first = mABLengths[pick_index][index];
		AxisAlignedBox ab;
		ab.merge(mAABBs[pick_index][index].mMaximum + offset);
		ab.merge(mAABBs[pick_index][index].mMinimum + offset);
		result.second = ab;
		return result;
	}

	AxisAlignedBox PavementModelUtil::GetPickModelABox(size_t pick_index, const Vector3& offset)
	{
		assert((pick_index < mBoxs.size()) && "out of range in mBox size");
		AxisAlignedBox ab;
		ab.merge(offset + mBoxs[pick_index].mMaximum);
		ab.merge(mBoxs[pick_index].mMinimum + offset);
		return ab;
	}

	int PavementModelUtil::GetIndexPillarCountSize(size_t index)
	{
		if (index >= mPillarCounts.size()) return -1;
		return (int)mPillarCounts[index].size();
	}

	void PavementModelUtil::AddRequests(WorkQueue::RequestID& id, size_t pick_index, bool isStep)
	{
		stringstream ss;
		ss << pick_index << "_" << isStep;
		mRequests[ss.str()] = id;
	}

	void PavementModelUtil::RecodeModelPoint(size_t index)
	{
		if (index < mMeshDatas.size())
		{
			std::shared_ptr<PavementTool::PavementUtilMesh>& MeshData = mMeshDatas[index];
			if (MeshData->mMeshData->Points.size() != 0)
				MeshData->mMeshData->Points.clear();

			std::vector<PavementTool::AsyncDataCache>& mPoints = MeshData->mMeshData->Points;
			std::shared_ptr<PavementTool::PrivateMeshData>& only_mesh = MeshData->mMeshData->only_mesh;

			for (size_t lodlevel = 0; lodlevel < only_mesh->SubData.size(); ++lodlevel)
			{
				PavementTool::AsyncDataCache tempCache;
				for (size_t sub_num = 0; sub_num < only_mesh->SubData[lodlevel].size(); ++sub_num)
				{
					std::vector<Echo::Vector3> subCalculPoints;
					const PavementTool::SubMeshData_& subdata = only_mesh->SubData[lodlevel][sub_num];
					for (size_t pointNum = 0; pointNum < subdata.road_vertexes.size(); ++pointNum)
					{
						Vector3 point = subdata.road_vertexes[pointNum].position;
						point *= MeshData->mModelChange[1];
						point = MeshData->mPointChangeMatrix * point;
						Vector3 moveyz = Vector3(0.f, MeshData->mModelChange[0].y, MeshData->mModelChange[0].z);
						point += moveyz;

						subCalculPoints.push_back(point);
					}
					tempCache.push_back(subCalculPoints);
					
				}
				mPoints.push_back(tempCache);
			}
		}
	}

	void PavementModelUtil::RecodeVertex(std::vector<Vector3>& vdata, const Vector3& position, const Vector3& scale, const Matrix3& mChangeMatrix)
	{
		std::vector<Vector3> data(vdata.begin(), vdata.end());
		vdata.clear();
		for (size_t pointNum = 0; pointNum < data.size(); ++pointNum)
		{
			Vector3 point = data[pointNum];
			point *= scale;

			point = mChangeMatrix * point;
			Vector3 moveyz = Vector3(0.f, position.y, position.z);
			point += moveyz;

			vdata.push_back(point);
		}
	}

	Echo::Vector3 PavementModelUtil::GetPoint(std::shared_ptr<PavementTool::PavementUtilMesh> mesh_d, size_t sub_num, size_t index, EM_LOD_LEVEL lodlevel)
	{
		//std::weak_ptr<PavementTool::AsyncData> tempData = mesh_d->mMeshData;
		const std::vector<PavementTool::AsyncDataCache>& tempData1 = mesh_d->mMeshData->Points;//变换计算后的顶点数据缓存
		const PavementTool::LodMeshDataCache& tempData2 = mesh_d->mMeshData->only_mesh->SubData;//原模型的顶点数据缓存
		const PavementTool::AsyncDataCache& points = tempData1[lodlevel].empty() ? tempData1.back() : tempData1[lodlevel];
		const PavementTool::SubMeshDataCache& subdata = tempData2[lodlevel].empty() ? tempData2.back() : tempData2[lodlevel];

		if (points[sub_num].size() > index)
			return points[sub_num][index];
		else
			return subdata[sub_num].road_vertexes[index].position;
		
	}

	void PavementModelUtil::InitRotateAngle(const std::vector<float>& angle)
	{
		mPointAngle = angle;
	}

	float PavementModelUtil::GetRealLength(size_t pick_index)
	{
		float pick_length = mBezierUtil->GetPartLength(pick_index);
		int segment = int(pick_length / GetPrivateMeshData(pick_index)->GetOffset().x);
		return pick_length / (float)segment;
	}

	float PavementModelUtil::GetRealStep(size_t pick_index)
	{
		float pick_length = mBezierUtil->GetPartLength(pick_index);
		int segment = int(pick_length / mSpacingStep);
		return pick_length / (float)segment;
	}

	void PavementModelUtil::setPavementRenderDistance(float distance)
	{
		for (auto& objVec : mModelMoveObjs)
		{
			for (auto& obj : objVec)
			{
				obj->setRenderDistance(mCullDistance);
			}
		}
	}

	void PavementModelUtil::setOrnamentRenderDistance(float distance)
	{
		for (auto& obj : mStepBatchObjs)
		{
			obj->mRenderDistance = distance;
		}
	}

	void PavementModelUtil::setPavementLodDistance(EM_LOD_LEVEL lodlevel, float distance)
	{
		for (auto& objVec : mModelMoveObjs)
		{
			for (auto& obj : objVec)
			{
				obj->setLodDistance(lodlevel, m_fLodDistance[lodlevel]);
			}
		}
	}

	void PavementModelUtil::setOrnamentLodDistance(EM_LOD_LEVEL lodlevel, float distance)
	{
		for (auto& obj : mStepBatchObjs)
		{
			obj->m_fLodDistance[lodlevel] = distance;
		}
	}

	//============================================================================================================
	// PavementPhyUtil
	PavementPhyUtil::PavementPhyUtil()
	{
		stringstream ss;
		ss << this;
		mNamePath = ss.str();

		RoadHelper::instance()->setPhyVaild(mNamePath, true, this);
	}

	PavementPhyUtil::~PavementPhyUtil()
	{
		for (auto& v : mRequests)
			RoadHelper::instance()->abortRequest(v.second);
		mRequests.clear();

		RoadHelper::instance()->setPhyVaild(mNamePath);
	}

	void PavementPhyUtil::DeleteModelPhy(size_t phy_index, bool del)
	{
		std::vector<MeshColliderComponentPtr>& vec = mPhyModel[phy_index];
		if (!vec.empty())
		{
			for (auto& com : vec)
			{
				if (!com.expired())
				{
					auto comInfo = com->m_pActComInfo;
					mActorPtr.getDynamicCastWeakPtr<PavementComponent>()->getActor()
						->delComponent(com.getStaticCastWeakPtr<ActorComponent>());
					mActorPtr.getDynamicCastWeakPtr<PavementComponent>()->getActor()
						->delComponentInfo(comInfo);
				}
			}
			if (del)
			{
				mPhyModel.erase(std::next(mPhyModel.begin(), phy_index));
				mPhyLens.erase(std::next(mPhyLens.begin(), phy_index));
			}

			/*ProtocolComponentManager::instance()->callProtocolFunc(
				Echo::PFID_Editor_SeleteComToEditor, this, (Echo::ProtocolHandle)this
			);*/
		}
	}

	void PavementPhyUtil::DeleteAllModelPhy()
	{
		for (size_t PhyIndex = 0; PhyIndex < mPhyLens.size(); ++PhyIndex)
			if (mPhyLens[PhyIndex].second != -1.f)
				DeleteModelPhy(PhyIndex);
	}

	void PavementPhyUtil::DecideCreatePhyMode(const AxisAlignedBox& role_box, const Vector3& offset)
	{
		AxisAlignedBox PickModelBox;

		for (size_t pick_index = 0; pick_index < mModelUtil->GetPickModelBoxCount(); ++pick_index)
		{
			PickModelBox = mModelUtil->GetPickModelABox(pick_index, offset); /*Boxs[pick_index];*/
			if (PickModelBox.Intersect(role_box))
			{
				for (size_t ab_index = 0; ab_index < mModelUtil->GetPickIndexABoxCount(pick_index); ++ab_index)
				{
					std::pair<float, AxisAlignedBox> value = mModelUtil->GetSegmentABox(pick_index, ab_index, offset);

					size_t PhyIndex = 0;
					for (PhyIndex = 0; PhyIndex < mPhyLens.size(); ++PhyIndex)
					{
						if (value.second.Intersect(role_box) &&
							mPhyLens[PhyIndex].first == pick_index &&
							mPhyLens[PhyIndex].second == value.first)
							break;
					}

					if (PhyIndex == mPhyLens.size() && value.second.Intersect(role_box))
					{
						mPhyLens.push_back(std::pair<size_t, float>(pick_index, value.first));
						mPhyModel.emplace_back();
						
						if (0 != mModelUtil->GetModeMoveCount() && value.first != -1.f && !Root::instance()->UsePavementTestNewMode)
						{
							AddRequestPhyModel(pick_index, ab_index, mPhyLens[PhyIndex]);
						}
						else if (0 != mModelUtil->GetSceneNodeCount() && value.first != -1.f && !Root::instance()->UsePavementTestNewMode)
						{
							AddRequestPhyStep(pick_index, ab_index, mPhyLens[PhyIndex]);
						}
					}
				}
			}
		}
	}

	bool PavementPhyUtil::DecideDeletePhyMode(const AxisAlignedBox& role_box, const Vector3& offset)
	{
		AxisAlignedBox ab;
		for (auto& a_b : mModelUtil->GetABoxs())
			ab.merge(a_b);

		ab.setMaximum(ab.mMaximum + offset);
		ab.setMinimum(ab.mMinimum + offset);

		if (ab.Intersect(role_box))
		{
			for (size_t PhyIndex = 0; PhyIndex < mPhyLens.size(); ++PhyIndex)
			{
				if (mPhyLens[PhyIndex].second != -1.f)
				{
					size_t road_index = mPhyLens[PhyIndex].first;
					std::vector<float> ABLengths = mModelUtil->GetABLengths(road_index);
					const auto& iter = std::find(ABLengths.begin(), ABLengths.end(), mPhyLens[PhyIndex].second);

					if (iter != ABLengths.end())
					{
						size_t boxindex = iter - ABLengths.begin();
						std::pair<float, AxisAlignedBox> mLBox = mModelUtil->GetSegmentABox(road_index, boxindex, offset);
						if (!mLBox.second.Intersect(role_box))
							DeleteModelPhy(PhyIndex);
					}
				}
				else
					DeleteModelPhy(PhyIndex);
			}
		}
		else
		{
			DeleteAllModelPhy();
			return true;
		}
		return false;
	}

	void PavementPhyUtil::AsyncCreatePhy(const std::pair<size_t, float>& IndexLens, const std::vector<std::vector<Vector3>>& VertexDatas, 
		const std::vector<std::vector<uint16>>& IndexDatas, const std::vector<AxisAlignedBox>& BoxDatas)
	{
		stringstream ss;
		ss << IndexLens.first << "_" << IndexLens.second << "_" << false;
		if (mRequests.find(ss.str()) == mRequests.end()) return;

		PavementComponentPtr pavement = mActorPtr.getDynamicCastWeakPtr<PavementComponent>();

		std::vector<MeshColliderComponentPtr> PhyModes;
		
		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = mModelUtil->GetPrivateMeshData(IndexLens.first);
		MeshPtr& mesh = MeshData->mMeshData->only_mesh->mesh;

		float Length = IndexLens.second;

		for (size_t i = 0; i < VertexDatas.size(); ++i)
		{
			MeshColliderComponentPtr pCom;
			if (i < PhyModes.size() && PhyModes[i] == nullptr)
				pCom = PhyModes[i];

			if (i < PhyModes.size() && PhyModes[i] != nullptr)
			{
				pCom = PhyModes[i];
				pCom->changeComName(
					Name(mesh->getName().toString() + "_"
						+ std::to_string(IndexLens.first) + "_"
						+ std::to_string(Length) + "_"
						+ std::to_string(i))
				);
			}
			else
			{
				pCom = pavement->getActor()->addComponent(Name("MeshColliderComponent")
					, mesh->getName().toString() + "_"
						+ std::to_string(IndexLens.first) + "_"
						+ std::to_string(Length) + "_"
						+ std::to_string(i)
					, pavement->weak_from_this(), nullptr, pavement->getGroupID()
				).getStaticCastWeakPtr<MeshColliderComponent>();
			}

			pCom->SetMeshData(VertexDatas[i], IndexDatas[i]);
			pCom->SetPhysicsLocalAABB(BoxDatas[i]);

			if (i >= PhyModes.size())
				PhyModes.push_back(pCom);
		}

		for (size_t i=0;i< mPhyLens.size();++i)
		{
			auto& len = mPhyLens[i];
			if (len.first == IndexLens.first && len.second == IndexLens.second)
			{
				if (PhyModes.size() == 0)
				{
					DeleteModelPhy(i);
				}
				else
				{
					if (mPhyModel[i].size() != 0)
						DeleteModelPhy(i, false);
					mPhyModel[i] = PhyModes;
				}

				break;
			}
		}

		
		mRequests.erase(ss.str());
	}

	void PavementPhyUtil::AsyncCreatePhyStep(const Vector3& offset, const std::pair<size_t, float>& IndexLens, 
		const std::vector<PavementTool::EntityData_*>& resultData, const Name& phyname)
	{
		stringstream ss;
		ss << IndexLens.first << "_" << IndexLens.second << "_" << true;
		if (mRequests.find(ss.str()) == mRequests.end()) return;
		PavementComponentPtr pavement = mActorPtr.getDynamicCastWeakPtr<PavementComponent>();

		std::vector<MeshColliderComponentPtr> PhyModes;

		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = mModelUtil->GetPrivateMeshData(IndexLens.first);
		MeshPtr& mesh = MeshData->mMeshData->only_mesh->mesh;
		float Length = IndexLens.second;

		for (size_t k = 0; k < resultData.size(); ++k)
		{
			MeshColliderComponentPtr pCom;
			{
				pCom = pavement->getActor()->addComponent(Name("MeshColliderComponent")
					, mesh->getName().toString() + "_"
						+ std::to_string(IndexLens.first) + "_"
						+ std::to_string(Length) + "_"
						+ std::to_string(k)
					, pavement->weak_from_this(), nullptr, pavement->getGroupID()
				).getStaticCastWeakPtr<MeshColliderComponent>();
			}

			Vector3 position = resultData[k]->position;

			pCom->SetMeshName_meshphy(phyname);
			pCom->setPosition(position);
			pCom->setScale(resultData[k]->otherValue);
			pCom->setRotation(resultData[k]->ori);

			PhyModes.push_back(pCom);

			position = position + offset;
			position.y -= MeshData->GetOffset().y;

			if (0 != mModelUtil->GetPillarMoveCount() && IndexLens.second != -1.f)
			{
				if (0 != mModelUtil->GetMaxPillarSum())
				{
					Ray ray(position, Vector3::NEGATIVE_UNIT_Y);
					float TerrainHeight = PiecewiseTool::calculateMinTerrainLength(ray, MeshData->GetOffset(), resultData[k]->tranformMatrix, mModelUtil->mPickModel);
					float HiLength = position.y - TerrainHeight;
					if (HiLength > 0)
					{
						float offsety = MeshData->GetOffset().y;
						int createSum = int(HiLength / offsety) + 1;
						if (createSum > mModelUtil->GetMaxPillarSum())
							createSum = mModelUtil->GetMaxPillarSum();
						for (int cNum = 1; cNum <= createSum; ++cNum)
						{
							Vector3 offsetPoint = MeshData->mModelChange[0];
							offsetPoint.y = offsetPoint.y - offsety * cNum;
							Vector3 offsetposition = resultData[k]->point + resultData[k]->tranformMatrix * offsetPoint;

							MeshColliderComponentPtr _Com;
							{
								_Com = pavement->getActor()->addComponent(Name("MeshColliderComponent")
									, mesh->getName().toString() + "_"
										+ std::to_string(IndexLens.first) + "_"
										+ std::to_string(HiLength) + "_"
										+ std::to_string(k) + "_"
										+ std::to_string(cNum)
									, pavement->weak_from_this(), nullptr, pavement->getGroupID()
								).getStaticCastWeakPtr<MeshColliderComponent>();
							}

							std::multimap<uint32, Name> mapPhyMeshName;
							mesh->getPhyMeshName(mapPhyMeshName);
							if (mapPhyMeshName.size() == 0)
								mapPhyMeshName.insert({ 0, mesh->getName() });
							for (auto& meshname : mapPhyMeshName)
							{
								Name PhyMeshName = PavementTool::GetDstPhyMeshName(meshname.second);
								_Com->SetMeshName_meshphy(PhyMeshName);
							}

							_Com->setPosition(offsetposition);
							_Com->setScale(resultData[k]->otherValue);
							_Com->setRotation(resultData[k]->ori);

							PhyModes.push_back(_Com);
						}
					}
				}
			}
		}

		for (size_t i = 0; i < mPhyLens.size(); ++i)
		{
			auto& len = mPhyLens[i];
			if (len.first == IndexLens.first && len.second == IndexLens.second)
			{
				if (PhyModes.size() == 0)
				{
					DeleteModelPhy(i);
				}
				else
				{
					if (mPhyModel[i].size() != 0)
						DeleteModelPhy(i, false);
					mPhyModel[i] = PhyModes;
				}

				break;
			}
		}

		mRequests.erase(ss.str());
	}

	void PavementPhyUtil::InitPhyMeshName(const Name& mesh, Name& phyname)
	{
		Name PhyMeshName = PavementTool::GetDstPhyMeshName(mesh);
		phyname = PhyMeshName;
	}

	void PavementPhyUtil::AddRequestPhyModel(size_t pick_index, size_t ab_index, std::pair<size_t, float>& IndexLens)
	{
		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = mModelUtil->GetPrivateMeshData(pick_index);

		const std::vector<float>& ABLength = mModelUtil->GetABLengths(pick_index);
		PavementComponentPtr pavement = mActorPtr.getDynamicCastWeakPtr<PavementComponent>();

		float NextLens = 0.f;
		if (pavement->getCreatePart() != -1 && pavement->getCreatePart() == (int)pick_index)
		{
			NextLens = mModelUtil->GetCreateLength();
			if (IndexLens.first != mModelUtil->GetSegmentPick()) return;
		}
		else if (pavement->getCreatePart() == -1)
		{
			if (ABLength.size() > ab_index + 1)
				NextLens = ABLength[ab_index + 1];
			else if (mModelUtil->GetPickModelBoxCount() >= pick_index + 1)
				NextLens = mModelUtil->GetBezierUtil()->GetPartLength(pick_index);
			else
				NextLens = ABLength[ab_index];
		}
		else
		{
			if (pavement->getCreatePart() > pick_index)
				NextLens = mModelUtil->GetBezierUtil()->GetPartLength(pick_index);
		}

		PiecewiseBezierUtil* mbezier = mModelUtil->GetBezierUtil();
		PavementTool::AsyncModelData ModelData;
		PavementModelUtil* pTool = mModelUtil;

		ModelData.from = mbezier->GetPoint(pick_index);
		ModelData.from_des = mbezier->GetDestination(pick_index);
		ModelData.to = mbezier->GetPoint(pick_index + 1);
		ModelData.to_sou = mbezier->GetSource(pick_index + 1);
		ModelData.from_angle = pTool->GetAngle(pick_index);
		ModelData.to_angle = pTool->GetAngle(pick_index + 1);

		Vector3 point, tangent, normal;
		if (pTool->GetUpdateWay() == 0)
			mbezier->GetBfData(pick_index, point, tangent, normal);

		PavementTool::CalcualteNormalData_ cnor;
		cnor.bf_point = point;
		cnor.bf_tangent = tangent;
		cnor.bf_normal = normal;
		cnor.normal1 = mbezier->GetNormal(pick_index);
		cnor.normal2 = mbezier->GetNormal(pick_index + 1);

		WorkQueue::RequestID id = RoadHelper::instance()->addCalculatePhyReq(mNamePath, NextLens, 
			MeshData, mbezier->GetPartLength(pick_index), mModelUtil->GetUpdateWay(), ModelData, 
			IndexLens, cnor);

		stringstream ss;
		ss << IndexLens.first << "_" << IndexLens.second;
		ss << "_" << false;
		mRequests[ss.str()] = id;
	}

	void PavementPhyUtil::AddRequestPhyStep(size_t pick_index, size_t ab_index, std::pair<size_t, float>& IndexLens)
	{
		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = mModelUtil->GetPrivateMeshData(pick_index);

		const std::vector<float>& ABLength = mModelUtil->GetABLengths(pick_index);
		PavementComponentPtr pavement = mActorPtr.getDynamicCastWeakPtr<PavementComponent>();

		float NextLens = 0.f;
		if (pavement->getCreatePart() != -1 && pavement->getCreatePart() == (int)pick_index)
		{
			NextLens = mModelUtil->GetCreateLength();
			if (IndexLens.first != mModelUtil->GetSegmentPick()) return;
		}
		else if (pavement->getCreatePart() == -1)
		{
			if (ABLength.size() > ab_index + 1)
				NextLens = ABLength[ab_index + 1];
			else if (mModelUtil->GetPickModelBoxCount() >= pick_index + 1)
				NextLens = mModelUtil->GetBezierUtil()->GetPartLength(pick_index);
			else
				NextLens = ABLength[ab_index];
		}
		else
		{
			if (pavement->getCreatePart() > pick_index)
				NextLens = mModelUtil->GetBezierUtil()->GetPartLength(pick_index);
		}

		PiecewiseBezierUtil* mbezier = mModelUtil->GetBezierUtil();
		PavementTool::AsyncModelData ModelData;
		PavementModelUtil* pTool = mModelUtil;

		ModelData.from = mbezier->GetPoint(pick_index);
		ModelData.from_des = mbezier->GetDestination(pick_index);
		ModelData.to = mbezier->GetPoint(pick_index + 1);
		ModelData.to_sou = mbezier->GetSource(pick_index + 1);
		ModelData.from_angle = pTool->GetAngle(pick_index);
		ModelData.to_angle = pTool->GetAngle(pick_index + 1);

		Vector3 point, tangent, normal;
		if (pTool->GetUpdateWay() == 0)
			mbezier->GetBfData(pick_index, point, tangent, normal);

		PavementTool::CalcualteNormalData_ cnor;
		cnor.bf_point = point;
		cnor.bf_tangent = tangent;
		cnor.bf_normal = normal;
		cnor.normal1 = mbezier->GetNormal(pick_index);
		cnor.normal2 = mbezier->GetNormal(pick_index + 1);

		WorkQueue::RequestID id = RoadHelper::instance()->addCalculatePhyStepReq(mNamePath, mModelUtil->GetSpacingStep()
			, mModelUtil->GetPickModel(), NextLens, MeshData, mbezier->GetPartLength(pick_index), mModelUtil->GetUpdateWay()
			, pavement->getWorldPosition(), ModelData, IndexLens, mModelUtil->GetPillarMoveCount() != 0, cnor);

		stringstream ss;
		ss << IndexLens.first << "_" << IndexLens.second;
		ss << "_" << true;
		mRequests[ss.str()] = id;
	}

	//--------------------------------------------------------------------------------------------------------------
	RoadHelper::RoadHelper()
	{
		mWorkQueue = Root::instance()->getWorkQueue();
		mWorkQueueChannel = mWorkQueue->getChannel("Echo/RoadHelper");
		mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->addResponseHandler(mWorkQueueChannel, this);

		InstanceStaticBatchObject::Init();
	}
	RoadHelper::~RoadHelper()
	{
		mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
		mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);

		mVaild.clear();
		mComVaild.clear();
		mPhyVaild.clear();

		InstanceStaticBatchObject::Destroy();
	}

	bool RoadHelper::isVaild()
	{
		return ms_instance != nullptr;
	}

	namespace
	{
		struct RoadMeshRequest
		{
			String pavement;
			Name mesh_name;
			size_t num;
			size_t InfoNum;

			friend std::ostream& operator << (std::ostream& o, const RoadMeshRequest& r) {
				return o;
			}
		};

		struct RoadCalculateRequest
		{
			SceneManager* sceneManager = nullptr;
			String pavement;
			PavementUtilMeshPtr mesh_d;
			size_t pick_index;
			Vector3 offset;
			int UpdateWay;
			float pick_length;
			PavementTool::AsyncModelData Data;

			PavementTool::CalcualteNormalData_ p_N;

			friend std::ostream& operator << (std::ostream& o, const RoadCalculateRequest& r) {
				return o;
			}
		};

		struct RoadCaculateStepReq 
		{
			SceneManager* sceneManager = nullptr;
			String pavement;
			PavementUtilMeshPtr mesh_d;
			size_t pick_index;
			float pick_length;
			float step;
			int UpdateWay;
			bool pickEntity;
			float segment_length;
			PavementTool::AsyncModelData ModelData;

			PavementTool::CalcualteNormalData_ p_N;

			friend std::ostream& operator << (std::ostream& o, const RoadCaculateStepReq& r) {
				return o;
			}
		};

		struct RoadPhyCalculateReq
		{
			String pavement;
			float NextLens;
			PavementUtilMeshPtr MeshData;
			float part_length;
			int UpdataWay;
			PavementTool::AsyncModelData ModelData;
			std::pair<size_t, float> IndexLens;

			PavementTool::CalcualteNormalData_ p_N;

			friend std::ostream& operator << (std::ostream& o, const RoadPhyCalculateReq& r) {
				return o;
			}
		};

		struct RoadPhyCalculateStepReq
		{
			String pavement; 
			float step_size;
			bool pickEntity;
			float NextLens;
			PavementUtilMeshPtr MeshData;
			float part_length;
			int UpdataWay;
			Vector3 offset;
			PavementTool::AsyncModelData ModelData;
			std::pair<size_t, float> IndexLens;
			bool havePillar;

			PavementTool::CalcualteNormalData_ p_N;

			friend std::ostream& operator << (std::ostream& o, const RoadPhyCalculateStepReq& r) {
				return o;
			}
		};

		struct ObjectMaterialRequest
		{
			InstanceMaterialListener* listener;
			MaterialWrapper mat;

			friend std::ostream& operator << (std::ostream& o, const ObjectMaterialRequest& r) {
				return o;
			}
		};

		struct RoadMeshResponse
		{
			String pavement;
			MeshPtr mesh;
			size_t num;
			size_t InfoNum;
			PavementTool::LodMeshDataCache data;

			friend std::ostream& operator << (std::ostream& o, const RoadMeshResponse& r) {
				return o;
			}
		};

		struct RoadCalculateResponse
		{
			String pavementutil;
			size_t pick_index;
			Vector3 offset;
			float length;
			std::vector<TrafficObject*> resuobj;
			std::vector<AxisAlignedBox> mABs;
			std::vector<float> mABLength;
			float mablength;

			friend std::ostream& operator << (std::ostream& o, const RoadCalculateResponse& r) {
				return o;
			}
		};

		struct RoadCaculateStepResp
		{
			String pavementutil;
			size_t pick_index;
			float step = 0.f;
			PavementTool::AABoxData_* data;

			friend std::ostream& operator << (std::ostream& o, const RoadCaculateStepResp& r) {
				return o;
			}
		};

		struct RoadPhyCalculateResp
		{
			String pavementphy;
			std::vector<std::vector<Vector3>> VertexDatas;
			std::vector<std::vector<uint16>> IndexDatas;
			std::pair<size_t, float> Lens;
			std::vector<AxisAlignedBox> mABs;

			friend std::ostream& operator << (std::ostream& o, const RoadPhyCalculateResp& r) {
				return o;
			}
		};

		struct RoadPhyCalculateStepResp
		{
			String pavementphy;
			std::pair<size_t, float> Lens;
			std::vector<PavementTool::EntityData_*> data;
			Vector3 offset;
			Name phyName;

			friend std::ostream& operator << (std::ostream& o, const RoadPhyCalculateStepResp& r) {
				return o;
			}
		};

		struct PlanetFitGroundRoadMaskCalculateResponse
		{
			//Name sceneName = Name::g_cEmptyName;
			SphericalTerrain* planet = nullptr;
			//std::vector<int> areaIDs;
			std::vector<AxisAlignedBox> boxVec;
			std::vector<Vector3> result;
			bool isRemove = true;
			friend std::ostream& operator << (std::ostream& o, const PlanetFitGroundRoadMaskCalculateResponse& r) {
				return o;
			}
		};
	}

	WorkQueue::RequestID RoadHelper::addMeshRequest(String& pavement, const Name& mesh, size_t num, size_t InfoNum, bool needData)
	{
		RoadMeshRequest req = { pavement, mesh, num, InfoNum };
		ReqType v = ReqType_LoadMesh;
		std::shared_ptr<PavementTool::PrivateMeshData> it_data;
		if (needData && !RoadManager::instance()->GetPathMeshData(mesh, it_data, needData))
			v = ReqType_LoadMeshData;

		return mWorkQueue->addRequest(mWorkQueueChannel, v, Any(req));
	}

	WorkQueue::RequestID RoadHelper::addObjectMatReq(
		class InstanceMaterialListener* listener, const MaterialWrapper& mat)
	{
		ObjectMaterialRequest req = { listener, mat };
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_ObjectMaterial, Any(req));
	}

	WorkQueue::RequestID RoadHelper::addCalculatePointReq(String& pavement, PavementUtilMeshPtr& MeshData,
		size_t pick_index, const Vector3& worldoffset, int UpdateWay, float pick_length,
		PavementTool::AsyncModelData& Data, PavementTool::CalcualteNormalData_& p_N, SceneManager* sceneManager)
	{
		RoadCalculateRequest req = { sceneManager, pavement, MeshData, pick_index, worldoffset, UpdateWay, pick_length, Data, p_N };
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_Calculate, Any(req));
	}

	WorkQueue::RequestID RoadHelper::addCalculateStepReq(String& pavement, PavementUtilMeshPtr& MeshData, size_t pick_index, float pick_length, float step,
		int UpdateWay, bool pickEntity, float segment_length, const PavementTool::AsyncModelData& ModelData, PavementTool::CalcualteNormalData_& p_N, SceneManager* sceneManager)
	{
		RoadCaculateStepReq req = { sceneManager, pavement, MeshData, pick_index, pick_length, step, UpdateWay, pickEntity, segment_length, ModelData, p_N };
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_StepCalculate, Any(req));
	}

	WorkQueue::RequestID RoadHelper::addCalculatePhyReq(String& pavement, float NextLens, PavementUtilMeshPtr& mesh,
		float part_length, int UpdateWay, const PavementTool::AsyncModelData& ModelData, 
		std::pair<size_t, float>& IndexLens, PavementTool::CalcualteNormalData_& p_N)
	{
		RoadPhyCalculateReq req = { pavement, NextLens, mesh, part_length, UpdateWay, ModelData, IndexLens, p_N };
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_PhyModel, Any(req));
	}

	WorkQueue::RequestID RoadHelper::addCalculatePhyStepReq(String& pavement, float step_size, bool pickEntity
		, float NextLens, PavementUtilMeshPtr& mesh, float part_length, int UpdateWay, const Vector3& offset
		, const PavementTool::AsyncModelData& ModelData, std::pair<size_t, float>& IndexLens, bool havePillar
		, PavementTool::CalcualteNormalData_& p_N)
	{
		RoadPhyCalculateStepReq req = { pavement, step_size, pickEntity, NextLens
			, mesh, part_length, UpdateWay, offset, ModelData, IndexLens, havePillar, p_N };
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_PhyModelStep, Any(req));
	}

	WorkQueue::RequestID RoadHelper::UpdatePavement(Pavement* pavement, ReqType updateType)
	{
		return mWorkQueue->addRequest(mWorkQueueChannel, updateType, Any(pavement));
	}

	WorkQueue::RequestID RoadHelper::UpdatePlanetPavementMask(const RoadRangeData& data)
	{
		//PlanetFitGroundRoadMaskCalculateRequest req = { planet, pavements };
		RoadRangeData req = data;
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_UpdatePlanetPavementMask, Any(req));
	}

	WorkQueue::RequestID RoadHelper::UpdatePlanetBuildingMask(const BuildingRangeData& data)
	{
		BuildingRangeData req = data;
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_UpdatePlanetBuildingMask, Any(req));
	}

	void RoadHelper::abortRequest(unsigned long long reqID)
	{
		//增加互斥锁，防止在异步请求处理的时候其他线程删除道路实例导致崩溃
		tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
		mWorkQueue->abortRequest(reqID);
	}

	void RoadHelper::setVaild(const String& path, bool isVaild, PavementModelUtil* pavement)
	{
		auto iter = mVaild.find(path);
		if (isVaild)
		{
			if (iter == mVaild.end())
				mVaild[path] = pavement;
			else if (iter->second == nullptr)
				mVaild[path] = pavement;
		}
		else
		{
			if (iter != mVaild.end())
				mVaild.erase(iter);
		}
	}

	void RoadHelper::setComVaild(const String& path, bool isVaild, ActorComponent* pavement)
	{
		auto iter = mComVaild.find(path);
		if (isVaild)
		{
			if (iter == mComVaild.end())
				mComVaild[path] = pavement;
			else if (iter->second == nullptr)
				mComVaild[path] = pavement;
		}
		else
		{
			if (iter != mComVaild.end())
				mComVaild.erase(iter);
		}
	}

	void RoadHelper::setPhyVaild(const String& path, bool isVaild, PavementPhyUtil* pavement)
	{
		auto iter = mPhyVaild.find(path);
		if (isVaild)
		{
			if (iter == mPhyVaild.end())
				mPhyVaild[path] = pavement;
			else if (iter->second == nullptr)
				mPhyVaild[path] = pavement;
		}
		else
		{
			if (iter != mPhyVaild.end())
				mPhyVaild.erase(iter);
		}
	}

	WorkQueue::Response* RoadHelper::handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		WorkQueue::Response* response = 0;
		uint16 type = req->getType();
		switch (type)
		{
		case ReqType_UpdatePavementRender:
		case ReqType_UpdatePavementPhy:
		case ReqType_UpdatePavement:
		{
			//增加互斥锁，防止在异步请求处理的时候其他线程删除道路实例导致崩溃
			tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
			if (!req->getAborted())
			{
				Pavement* pavement = any_cast<Pavement*>(req->getData());
				if (!pavement)
				{
					response = new WorkQueue::Response(req, false, Any(pavement));
					break;
				}
				std::shared_ptr<PavementTool::PrivateMeshData> it_data = nullptr;
				
				it_data = RoadManager::instance()->prepareMeshData(pavement->mBaseMeshName);
				
				if (it_data != nullptr)
				{
					//SphericalTerrain* mPlanet = nullptr;
					//if (pavement->getIsFitGroundModel())
					//{
					//	for (auto& planet : pavement->getPavementComponent()->getSceneManager()->getPlanetManager()->m_SphericalTerrainSet)
					//	{
					//		if (!mPlanet)
					//		{
					//			mPlanet = planet;
					//		}
					//		else
					//		{
					//			DVector3 pos = pavement->getPavementComponent()->getWorldPosition();
					//			if (pos.squaredDistance(mPlanet->m_pos) > pos.squaredDistance(planet->m_pos))
					//				mPlanet = planet;
					//		}
					//	}
					//	assert(mPlanet && "Pavement FitGroundModel:: There is no planet.");
					//	//if (!mPlanet)
					//	//{
					//	//	LogManager::instance()->logMessage("Pavement FitGroundModel:: There is no planet.");
					//	//	//return;//暂时不返回，以不贴地模式更新
					//	//}
					//}
					if (!pavement->getIsCurveModel())
					{
						pavement->ResetBacthObject(it_data->mesh);
						pavement->CalculateObjectTransform();//不随曲线扭曲模型，计算间隔生成在曲线上的位置
					}
					else
					{
						if (type & ReqType_UpdatePavementRender)
							pavement->CalculateRenderVBIB(it_data);
						if (type & ReqType_UpdatePavementPhy)
							pavement->CalculatePhyVBIB(it_data);
					}
					response = new WorkQueue::Response(req, true, Any(pavement), "", "UpdatePavemenet");
				}
				else
					response = new WorkQueue::Response(req, false, Any(pavement));
			}

		}break;
		case ReqType_UpdatePlanetPavementMask:
		{
			if (!RoadManager::instance()->usePlanetRoadMask)
				break;
			tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
			if (!req->getAborted())
			{
				const RoadRangeData& data = any_cast<RoadRangeData>(req->getData());
				if (!data.planet || data.roads.empty())
				{
					response = new WorkQueue::Response(req, false, Any());
					break;
				}
				std::vector<Vector3> vbVec;
				std::vector<uint16> ibVec;

				RoadManager::instance()->CalculateRoadRange(data, vbVec, ibVec);
				
				if (vbVec.empty() || ibVec.empty())
				{
					response = new WorkQueue::Response(req, false, Any());
					break;
				}
				std::vector<Vector3> result = RoadManager::instance()->CalculatePlanetMask(data.planet, vbVec, ibVec);
				if (result.empty())
				{
					response = new WorkQueue::Response(req, false, Any());
					break;
				}
				PlanetFitGroundRoadMaskCalculateResponse resp = { data.planet, data.boxs, result, data.removeVeg };
				response = new WorkQueue::Response(req, true, Any(resp), "", "UpdatePlanetPavementMask");
			}
		}break;
		case ReqType_UpdatePlanetBuildingMask:
		{
			tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
			if (!req->getAborted())
			{
				const BuildingRangeData& data = any_cast<BuildingRangeData>(req->getData());
				if (!data.planet || data.builds.empty())
				{
					response = new WorkQueue::Response(req, false, Any());
					break;
				}
				std::vector<Vector3> vbVec;
				std::vector<uint16> ibVec;
				std::vector<AxisAlignedBox> boxs;
				auto convertLocalToWorldPosition = [](const Vector3& LocalPos, const Vector3& parentWorldPos, const Quaternion& parentWorldQuat)
				{
					return parentWorldPos + parentWorldQuat * LocalPos;
				};

				for(auto& build : data.builds)
				{
					uint16 count = (uint16)vbVec.size();
					Vector3 p1 = convertLocalToWorldPosition(build.box.vMin, build.worldPos, build.worldQuat);
					Vector3 p2 = convertLocalToWorldPosition(Vector3(build.box.vMin.x, build.box.vMin.y, build.box.vMax.z), build.worldPos, build.worldQuat);
					Vector3 p3 = convertLocalToWorldPosition(Vector3(build.box.vMax.x, build.box.vMin.y, build.box.vMax.z), build.worldPos, build.worldQuat);
					Vector3 p4 = convertLocalToWorldPosition(Vector3(build.box.vMax.x, build.box.vMin.y, build.box.vMin.z), build.worldPos, build.worldQuat);

					vbVec.push_back(p1);
					vbVec.push_back(p2);
					vbVec.push_back(p3);
					vbVec.push_back(p4);

					ibVec.push_back(count);
					ibVec.push_back(count + 1);
					ibVec.push_back(count + 3);

					ibVec.push_back(count + 1);
					ibVec.push_back(count + 2);
					ibVec.push_back(count + 3);

					AxisAlignedBox box = build.box;
					Matrix4 meshtransform;
					meshtransform.makeTransform(build.worldPos, Vector3::UNIT_SCALE, build.worldQuat);
					box.transformAffine(meshtransform);
					boxs.push_back(box);
				}

				if (vbVec.empty() || ibVec.empty())
				{
					response = new WorkQueue::Response(req, false, Any());
					break;
				}
				std::vector<Vector3> result = RoadManager::instance()->CalculatePlanetMask(data.planet, vbVec, ibVec);
				if (result.empty())
				{
					response = new WorkQueue::Response(req, false, Any());
					break;
				}
				PlanetFitGroundRoadMaskCalculateResponse resp = { data.planet, boxs, result, data.removeVeg };
				response = new WorkQueue::Response(req, true, Any(resp), "", "UpdatePlanetPavementMask");
			}
		}
		break;
		case ReqType_LoadMesh:
		{
			const RoadMeshRequest* MeshReq = any_cast<RoadMeshRequest>(&req->getData());
			if (!req->getAborted())
			{
				try
				{
					MeshPtr mesh = Echo::MeshMgr::instance()->prepare(MeshReq->mesh_name);

					PavementTool::LodMeshDataCache data;
					RoadMeshResponse resp = { MeshReq->pavement, mesh, MeshReq->num, MeshReq->InfoNum, data };
					response = new WorkQueue::Response(req, true, Any(resp), MeshReq->mesh_name.toString());
				}
				catch (const Exception& excep)
				{
					response = new WorkQueue::Response(req, true, Any(RoadMeshResponse{ MeshReq->pavement, MeshPtr(), MeshReq->num, MeshReq->InfoNum }), "", MeshReq->mesh_name.toString());
					std::string log_str;
					log_str.append(MeshReq->mesh_name.toString()).
						append(" ").append(excep.getFullDescription());
					LogManager::instance()->logMessage(log_str);
				}
			}
		} break;
		case ReqType_LoadMeshData:
		{

			const RoadMeshRequest* MeshReq = any_cast<RoadMeshRequest>(&req->getData());
			if (!req->getAborted())
			{
				PavementTool::LodMeshDataCache vecdata;
				try
				{
					MeshPtr mesh = Echo::MeshMgr::instance()->prepare(MeshReq->mesh_name);

					auto getMeshData = [&](const Mesh& meshTemp)
					{
						uint32 num = meshTemp.getNumSubMesh();
						std::vector<PavementTool::SubMeshData_> tempVec;
						tempVec.reserve(num);
						for (uint32 sub = 0; sub < num; ++sub)
						{
							Echo::SubMesh* submesh = meshTemp.getSubMesh(sub);

							MeshVertexIndexData data;
							submesh->getVertexIndexData(data);
							PavementTool::SubMeshData_ subdata;

							for (size_t j = 0; j < data.vertices.size(); ++j)
							{
								PavementTool::Vertex_ value(data.vertices[j], data.normals[j]);

								subdata.road_vertexes.push_back(value);
								subdata.road_uv.push_back(data.uvs[j]);
								subdata.road_tangent.push_back(data.tangents[j]);
							}

							for (size_t j = 0; j < data.indices.size(); ++j)
								subdata.road_indices.push_back(data.indices[j]);

							tempVec.push_back(subdata);
						}
						vecdata.push_back(tempVec);
					};

					//运行时mesh在load阶段会释放掉缓存的顶点数据，需要重新序列化一遍
					if (mesh->isLoaded())
					{
						auto rePrepareMesh = [&](const Name& meshName) {
							Mesh tempMesh(meshName);
							tempMesh.prepare();
							DataStreamPtr stream(Root::instance()->GetMemoryDataStream(meshName.c_str()));

							if (stream.isNull() || stream->size() == 0)
							{
								LogManager::instance()->logMessage("[PavementUtil] get mesh resource failed!  meshName:"
									+ meshName.toString() + __FUNCTION__);
								response = new WorkQueue::Response(req, false, Any());
								return false;
								//break;
							}

							try
							{
								MeshSerializer serializer;
								serializer.importMesh(stream, &tempMesh);
								getMeshData(tempMesh);
								return true;
							}
							catch (...)
							{
								LogManager::instance()->logMessage("[PavementUtil] Mesh serializer import mesh resource failed!  meshName:"
									+ meshName.toString() + __FUNCTION__);
								response = new WorkQueue::Response(req, false, Any());
								return false;
								//break;
							}
						};
						
						if(!rePrepareMesh(mesh->getName()))
							return response;
						for (auto& lodmesh : mesh->m_vtLodMesh)
						{
							if(!rePrepareMesh(lodmesh->getName()))
								return response;
						}
					}
					else
					{
						getMeshData(*mesh.getPointer());
						for (auto& lodmesh : mesh->m_vtLodMesh)
						{
							getMeshData(*lodmesh.getPointer());
						}
					}
					
					RoadMeshResponse resp = { MeshReq->pavement, mesh, MeshReq->num, MeshReq->InfoNum, vecdata };

					response = new WorkQueue::Response(req, true, Any(resp), "", MeshReq->mesh_name.toString());
				}
				catch (const Exception& excep)
				{
					response = new WorkQueue::Response(req, true, Any(RoadMeshResponse{ MeshReq->pavement, MeshPtr(), MeshReq->num, MeshReq->InfoNum, vecdata}), "", MeshReq->mesh_name.toString());
					std::string log_str;
					log_str.append(MeshReq->mesh_name.toString()).
						append(" ").append(excep.getFullDescription());
					LogManager::instance()->logMessage(log_str);
				}
			}
		} break;
		case ReqType_ObjectMaterial:
		{
			const ObjectMaterialRequest* matReq = any_cast<ObjectMaterialRequest>(&req->getData());
			if (!req->getAborted())
			{
				if (matReq->mat.isV1())
				{
					matReq->mat.getV1()->prepareRes();
				}
				else if (matReq->mat.isV2())
				{
					matReq->mat.getV2()->prepareRes();
				}
				response = new WorkQueue::Response(req, true, Any(), "", matReq->mat.getName().toString());
			}
		}
		break;
		case ReqType_Calculate:
		{
			const RoadCalculateRequest* CalReq = any_cast<RoadCalculateRequest>(&req->getData());
			if (!req->getAborted())
			{
				try {
					size_t pick_index = CalReq->pick_index;
					std::vector<TrafficObject*> resuobj;
					std::vector<AxisAlignedBox> mABs;
					std::vector<float> mABLength;
					float mablength = 0.f;

					PavementTool::UpdateAsyncModelPart(pick_index, CalReq->pick_length
						, CalReq->UpdateWay, resuobj, CalReq->Data, CalReq->mesh_d, mABs
						, CalReq->p_N, mABLength, mablength, -1, CalReq->sceneManager);

					RoadCalculateResponse resp = { CalReq->pavement, pick_index, CalReq->offset,
						CalReq->pick_length, resuobj, mABs, mABLength, mablength};
					response = new WorkQueue::Response(req, true, Any(resp), "", "Road Calculate...");
					//LogManager::instance()->logMessage(LML_CRITICAL, "UpdateAsyncModelPart");
				}
				catch (const Exception& excep)
				{
					response = new WorkQueue::Response(req, false, Any());
					std::string log_str;
					log_str.append(String("ReqType_Calculate")).
						append(" ").append(excep.getFullDescription());
					LogManager::instance()->logMessage(log_str);
				}
			}
		} break;
		case ReqType_StepCalculate:
		{
			const RoadCaculateStepReq* CalReq = any_cast<RoadCaculateStepReq>(&req->getData());
			if (!req->getAborted())
			{
				size_t pick_index = CalReq->pick_index;
				PavementTool::AABoxData_* data = new PavementTool::AABoxData_();

				PavementTool::UpdateAsyncModelStep(pick_index, CalReq->pick_length, CalReq->step,
					CalReq->UpdateWay, CalReq->mesh_d, CalReq->pickEntity, CalReq->segment_length, 
					CalReq->ModelData, data, CalReq->p_N, -1, CalReq->sceneManager);

				RoadCaculateStepResp resp = { CalReq->pavement, CalReq->pick_index, CalReq->step, data };
				response = new WorkQueue::Response(req, true, Any(resp), "", "Road StepCalculate...");
			}
		} break;
		case ReqType_PhyModel:
		{
			const RoadPhyCalculateReq* CalReq = any_cast<RoadPhyCalculateReq>(&req->getData());
			if (!req->getAborted())
			{
				std::unordered_map<float, float> data;
				std::vector<std::vector<Vector3>> VertexDatas;
				std::vector<std::vector<uint16>> IndexDatas;
				std::pair<size_t, float> Lens = CalReq->IndexLens;
				std::vector<AxisAlignedBox> mBoxs;

				PavementTool::AsyncPhyModel(data, CalReq->NextLens, CalReq->MeshData,
					CalReq->part_length, CalReq->UpdataWay, CalReq->ModelData,
					Lens, VertexDatas, IndexDatas, CalReq->p_N, mBoxs);

				RoadPhyCalculateResp resp = { CalReq->pavement, VertexDatas, IndexDatas, Lens, mBoxs };
				response = new WorkQueue::Response(req, true, Any(resp), "", "Road PhyModel...");
			}
		} break;
		case ReqType_PhyModelStep:
		{
			const RoadPhyCalculateStepReq* CalReq = any_cast<RoadPhyCalculateStepReq>(&req->getData());
			if (!req->getAborted())
			{
				try {
					std::unordered_map<float, float> StoT;
					std::pair<size_t, float> Lens = CalReq->IndexLens;
					std::vector<PavementTool::EntityData_*> data;
					Name phyName;

					PavementTool::AsyncCalculatePhyStep(StoT, CalReq->step_size, CalReq->pickEntity
						, CalReq->NextLens, CalReq->MeshData, CalReq->part_length
						, CalReq->UpdataWay, CalReq->offset, CalReq->ModelData, Lens, data
						, phyName, CalReq->havePillar, CalReq->p_N);

					RoadPhyCalculateStepResp resp = { CalReq->pavement, Lens, data, CalReq->offset, phyName };
					response = new WorkQueue::Response(req, true, Any(resp), "", "Road PhyModelStep...");
				}
				catch (...)
				{

				}
			}
		} break;
		default:
			assert(false && "Unknown request type in InstanceMaterialHelper::handleRequest");
			response = new WorkQueue::Response(req, false, Any());
			break;
		}
		return response;
	}

	void RoadHelper::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		//if (!res->succeeded())
		//	return;

		const WorkQueue::Request* req = res->getRequest();
		uint16 type = req->getType();
		switch (type)
		{
		case ReqType_UpdatePavementRender:
		case ReqType_UpdatePavementPhy:
		case ReqType_UpdatePavement:
		{
			if (!req->getAborted())
			{
				Pavement* pavement = any_cast<Pavement*>(res->getData());
				if (!pavement)
				{
					break;
				}
				if (!res->succeeded())
				{
					pavement->m_setRenderTickets.erase(req->getID());
					pavement->m_setPhysicsTickets.erase(req->getID());
					if (pavement->mParent->getWorldManager())
						pavement->mParent->getWorldManager()->removePavementWorkObject(req->getID(), "Pavement response fail");
					break;
				}
				pavement->CalculateOperationCompleted(req->getID(), type);

			}
		}break;
		case ReqType_UpdatePlanetPavementMask:
		case ReqType_UpdatePlanetBuildingMask:
		{
			if (!res->succeeded() || !RoadManager::instance()->usePlanetRoadMask)
				break;
			if (!req->getAborted())
			{
				const PlanetFitGroundRoadMaskCalculateResponse* data = any_cast<PlanetFitGroundRoadMaskCalculateResponse>(&res->getData());

				if (!data || !data->planet)
				{
					break;
				}
				RoadManager::instance()->operationCompleted_PlanetMask(req->getID(), data->planet, data->result, data->boxVec, data->isRemove);

			}
		}break;
		case ReqType_LoadMesh:
		case ReqType_LoadMeshData:
		{
			const RoadMeshResponse* pResp = any_cast<RoadMeshResponse>(&res->getData());
			MeshPtr mesh;
			if (!req->getAborted() && pResp)
			{
				try
				{
					if (!pResp->mesh.isNull())
					{
						//pResp->mesh->load();
						mesh = pResp->mesh;
					}
				}
				catch (const Exception& excep)
				{
					std::string log_str;
					log_str.append(pResp->mesh->getName().toString()).
						append(" ").append(excep.getFullDescription());
					LogManager::instance()->logMessage(log_str);
				}
				//InstanceBatchBuilding::textureLoaded(pResp->build_id, tex);
				if (!mesh.isNull() && mComVaild.find(pResp->pavement) != mComVaild.end())
				{
					PavementComponent* pa = static_cast<PavementComponent*>(mComVaild[pResp->pavement]);
					PrivateMeshSharedPtr data(new PavementTool::PrivateMeshData());
					data->mesh = mesh;
					data->SubData = pResp->data;
					for (auto& iter : data->SubData)
					{
						data->vertexNumMax = data->vertexNumMax >= iter.size() ? data->vertexNumMax : iter.size();
					}
					RoadManager::instance()->SetPathMeshData(mesh->getName(), data);
					pa->tryInitPavementUtil(pResp->num, pResp->InfoNum);
				}
			}
		} break;
		case ReqType_ObjectMaterial:
		{
			const ObjectMaterialRequest* pReq = any_cast<ObjectMaterialRequest>(&req->getData());
			if (!req->getAborted())
			{
				if (pReq->mat.isV1())
				{
					pReq->mat.getV1()->loadInfo();
				}
				else if (pReq->mat.isV2())
				{
					pReq->mat.getV2()->load();
				}
				pReq->listener->materialLoaded(pReq->mat, req->getID());
			}
		}
		break;
		case ReqType_Calculate:
		{
			const RoadCalculateResponse* pResp = any_cast<RoadCalculateResponse>(&res->getData());
			if (!req->getAborted() && pResp && mVaild.find(pResp->pavementutil) != mVaild.end())
			{
				mVaild[pResp->pavementutil]->UpdateAsync(pResp->pick_index, pResp->offset, pResp->length, pResp->resuobj
					, pResp->mABs, pResp->mABLength, pResp->mablength);
			}
			else if (pResp)
			{
				for (auto& v : pResp->resuobj)
					delete v;
			}
		}break;
		case ReqType_StepCalculate:
		{
			const RoadCaculateStepResp* pResp = any_cast<RoadCaculateStepResp>(&res->getData());
			if (!req->getAborted() && pResp && mVaild.find(pResp->pavementutil) != mVaild.end())
			{
				mVaild[pResp->pavementutil]->UpdateAsyncStep(pResp->data, pResp->pick_index, pResp->step);
			}
			else if (pResp && pResp->data)
			{
				delete pResp->data->obj;
			}

			if (pResp)
			{
				delete pResp->data;
			}

		} break;
		case ReqType_PhyModel:
		{
			const RoadPhyCalculateResp* pResp = any_cast<RoadPhyCalculateResp>(&res->getData());
			if (!req->getAborted() && pResp && mPhyVaild.find(pResp->pavementphy) != mPhyVaild.end())
			{
				if (pResp->VertexDatas.size() != 0)
				{
					mPhyVaild[pResp->pavementphy]->AsyncCreatePhy(pResp->Lens, pResp->VertexDatas, pResp->IndexDatas, pResp->mABs);
				}
			}

		} break;
		case ReqType_PhyModelStep:
		{
			const RoadPhyCalculateStepResp* pResp = any_cast<RoadPhyCalculateStepResp>(&res->getData());
			if (!req->getAborted() && pResp && mPhyVaild.find(pResp->pavementphy) != mPhyVaild.end())
			{
				mPhyVaild[pResp->pavementphy]->AsyncCreatePhyStep(pResp->offset, pResp->Lens, pResp->data, pResp->phyName);
			}

			if (pResp)
				for (auto& v : pResp->data)
					delete v;

		} break;
		default:
			break;
		}
	}
}
