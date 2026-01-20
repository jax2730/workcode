///////////////////////////////////////////////////////////////////////////////
//
// @file EchoInstanceBatchEntity.cpp
// @author luoshuquan
// @date 08/13/2019 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "EchoStableHeaders.h"
#include "EchoInstanceBatchEntity.h"
#include "EchoRoot.h"
#include "EchoRenderSystem.h"
#include "EchoShadowmapManager.h"
#include "EchoEntity.h"
#include "EchoSubMesh.h"
#include "EchoMaterialManager.h"
#include "EchoInstanceMaterialHelper_p.h"
#include "EchoPointLightShadowManager.h"
#include "EchoTextureManager.h"
#include "EchoMaterialImplV2.h"
#include "EchoMaterialV2Manager.h"

namespace Echo
{

	enum InstanceType
	{
		eIT_None,
		eIT_Illum,
		eIT_IllumPBR,
		eIT_IllumPBR2022,
		eIT_IllumPBR2023,
		eIT_SpecialIllumPBR,
		eIT_BillboardLeaf,
		eIT_BillboardLeafPBR,
		eIT_Trunk,
		eIT_TrunkPBR,
		eIT_SimpleTreePBR,
		eIT_BaseWhiteNoLight,

		eIT_MaxCount,
	};

	enum InstanceDataType
	{
		eIDD_WorldTransform,
	};

	struct InstanceDataDesc
	{
		InstanceDataType type;
		uint32 offset;
		uint32 length;
	};

	union uniform_f4
	{
		struct
		{
			float x, y, z, w;
		};
		struct
		{
			float r, g, b, a;
		};

	};

	using Uniformf4Vec = std::vector<uniform_f4>;

	namespace
	{
		void uniform_f4_assign_pos(uniform_f4& uniform, const Vector3& pos)
		{
			EchoZoneScoped;

			uniform.x = pos.x;
			uniform.y = pos.y;
			uniform.z = pos.z;
		}

		void uniform_f4_assign_scale(uniform_f4& uniform, float fScale)
		{
			EchoZoneScoped;

			uniform.w = fScale;
		}

		void uniform_f4_assign_scale(uniform_f4& uniform, const Vector3& vScale)
		{
			EchoZoneScoped;

			uniform.x = vScale.x;
			uniform.y = vScale.y;
			uniform.z = vScale.z;
		}

		void uniform_f4_assign_orientation(uniform_f4& uniform, const Quaternion& qua)
		{
			EchoZoneScoped;

			uniform.x = qua.x;
			uniform.y = qua.y;
			uniform.z = qua.z;
			uniform.w = qua.w;
		}

		template <class T>
		T non_zero_round(T v, T threshold)
		{
			EchoZoneScoped;

			const auto zero_value = T(0);
			if (v < zero_value)
				v = v <= -threshold ? v : -threshold;
			else
				v = v >= threshold ? v : threshold;
			return v;
		}

		void updateWorldPositionScaleOrientation(const SubEntityVec& vecInst, uint32 iUniformCount,
			Uniformf4Vec& vecData)
		{
			EchoZoneScoped;

			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount, { { 0.f, 0.f, 0.f, 0.f } });

			DVector3 camPos = DVector3::ZERO;
			if (iInstNum != 0)
			{
				Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
				camPos = pCam->getDerivedPosition();
			}

			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				DVector3 position;
				Vector3 scale;
				Quaternion orientation;
				_world_matrix->decomposition(position, scale, orientation);
				position -= camPos; // camera-relative
				scale.x = non_zero_round(scale.x, 0.00001f);
				scale.y = non_zero_round(scale.y, 0.00001f);
				scale.z = non_zero_round(scale.z, 0.00001f);
				uniform_f4* _inst_buff = &vecData[i * iUniformCount];
				Vector3 tempPos = position; // cast to float3
				uniform_f4_assign_pos(_inst_buff[0], tempPos);
				uniform_f4_assign_scale(_inst_buff[1], scale);
				uniform_f4_assign_orientation(_inst_buff[2], orientation);
			}
		}

		void updateWorldPositionScaleOrientationOri(const SubEntityVec& vecInst, uint32 iUniformCount,
			Uniformf4Vec& vecData)
		{
			EchoZoneScoped;

			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount, { { 0.f, 0.f, 0.f, 0.f } });
			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				DVector3 position;
				Vector3 scale;
				Quaternion orientation;
				_world_matrix->decomposition(position, scale, orientation);
				scale.x = non_zero_round(scale.x, 0.00001f);
				scale.y = non_zero_round(scale.y, 0.00001f);
				scale.z = non_zero_round(scale.z, 0.00001f);
				uniform_f4* _inst_buff = &vecData[i * iUniformCount];
				uniform_f4_assign_pos(_inst_buff[0], position);
				uniform_f4_assign_scale(_inst_buff[1], scale);
				uniform_f4_assign_orientation(_inst_buff[2], orientation);
			}
		}
		



		void updateWorldTransform(const SubEntityVec& vecInst, uint32 iUniformCount,
			Uniformf4Vec& vecData)
		{
			EchoZoneScoped;

			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount, { { 0.f, 0.f, 0.f, 0.f } });

			// Get camera position for camera-relative transform (high precision)
			DVector3 camPos = DVector3::ZERO;
			if (iInstNum != 0)
			{
				Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
				camPos = pCam->getDerivedPosition();
			}

			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);

				// Apply camera-relative transform in double precision to preserve accuracy
				DBMatrix4 world_matrix_camera_relative = *_world_matrix;
				world_matrix_camera_relative[0][3] -= camPos[0];
				world_matrix_camera_relative[1][3] -= camPos[1];
				world_matrix_camera_relative[2][3] -= camPos[2];

				uniform_f4* _inst_buff = &vecData[i * iUniformCount];
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)world_matrix_camera_relative.m[i][0];
					_inst_buff[i].y = (float)world_matrix_camera_relative.m[i][1];
					_inst_buff[i].z = (float)world_matrix_camera_relative.m[i][2];
					_inst_buff[i].w = (float)world_matrix_camera_relative.m[i][3];  // Now small values
				}				//Vector3 position, scale;
					//Quaternion orientation;
					//_world_matrix->decomposition(position, scale, orientation);
				DBMatrix4 inv_transpose_world_matrix;
				const DBMatrix4* inv_world_matrix = nullptr;
				pSubEntity->getInvWorldTransforms(&inv_world_matrix);
				//inv_transpose_world_matrix.makeInverseTransform(position, scale, orientation);
				inv_transpose_world_matrix = inv_world_matrix->transpose();
				_inst_buff += 3;
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)inv_transpose_world_matrix.m[i][0];
					_inst_buff[i].y = (float)inv_transpose_world_matrix.m[i][1];
					_inst_buff[i].z = (float)inv_transpose_world_matrix.m[i][2];
					_inst_buff[i].w = (float)inv_transpose_world_matrix.m[i][3];
				}
			}
		}
		void updateWorldViewTransform(const SubEntityVec& vecInst, uint32 iUniformCount,
			Uniformf4Vec& vecData)
		{
			EchoZoneScoped;
			// copy form echoinstanceEntity.cpp func_updateWorldTransform
			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount);

			DVector3 camPos = DVector3::ZERO;
			if (iInstNum != 0)
			{
				Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
				camPos = pCam->getDerivedPosition();
			}

			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				DBMatrix4 world_matrix_not_cam = *_world_matrix;
				world_matrix_not_cam[0][3] -= camPos[0];
				world_matrix_not_cam[1][3] -= camPos[1];
				world_matrix_not_cam[2][3] -= camPos[2];
				uniform_f4* _inst_buff = &vecData[i * iUniformCount];
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)world_matrix_not_cam.m[i][0];
					_inst_buff[i].y = (float)world_matrix_not_cam.m[i][1];
					_inst_buff[i].z = (float)world_matrix_not_cam.m[i][2];
					_inst_buff[i].w = (float)world_matrix_not_cam.m[i][3];
				}

				//Vector3 position, scale;
				//Quaternion orientation;
				//_world_matrix->decomposition(position, scale, orientation);
				DBMatrix4 inv_transpose_world_matrix;
				const DBMatrix4* inv_world_matrix = nullptr;
				pSubEntity->getInvWorldTransforms(&inv_world_matrix);
				//inv_transpose_world_matrix.makeInverseTransform(position, scale, orientation);
				inv_transpose_world_matrix = inv_world_matrix->transpose();
				_inst_buff += 3;
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)inv_transpose_world_matrix.m[i][0];
					_inst_buff[i].y = (float)inv_transpose_world_matrix.m[i][1];
					_inst_buff[i].z = (float)inv_transpose_world_matrix.m[i][2];
					_inst_buff[i].w = (float)inv_transpose_world_matrix.m[i][3];
				}
			}
		}

		
	}

	static unsigned int _mipmap_level(unsigned int _size)
	{
		EchoZoneScoped;

		unsigned int _mipmap = 0u;
		while (_size >> _mipmap > 0) ++_mipmap;
		return _mipmap;
	}

	void _cat_tex_file_ext(char* tex_path)
	{
		EchoZoneScoped;

		char* ext = strrchr(tex_path, '.');
		if (nullptr == ext)
			return;

		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
		if (ECHO_DEVICE_DX11 == eClass) {
			strcpy(ext, ".dds");
		}
		else if (ECHO_DEVICE_GLES3 == eClass || ECHO_DEVICE_METAL == eClass) {
			const ECHO_TEX_TYPE renderTexType = Root::instance()->getRenderTextureType();
			if (ECHO_TEX_TYPE_DXT == renderTexType)
			{
				strcpy(ext, ".dds");
			}
			else if (ECHO_TEX_TYPE_ASTC == renderTexType ||
				ECHO_TEX_TYPE_ETC == renderTexType)
			{
				strcpy(ext, ".ktx");
			}
		}
	}

	ECHO_FMT _map_tex_format(ECHO_FMT src)
	{
		EchoZoneScoped;

		ECHO_FMT dst = src;
		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
		if (eClass == ECHO_DEVICE_DX11)
		{
			switch (src)
			{
			case ECHO_FMT_RGB8_ETC2:		dst = ECHO_FMT_DDS_BC1;	break;
			case ECHO_FMT_RGBA8_ETC2_EAC:	dst = ECHO_FMT_DDS_BC3;	break;
			default:						dst = src;					break;
			}
		}
		else if (eClass == ECHO_DEVICE_GLES3 || eClass == ECHO_DEVICE_METAL)
		{
			switch (src)
			{
			case ECHO_FMT_DDS_BC1:	dst = ECHO_FMT_RGB8_ETC2;		break;
			case ECHO_FMT_DDS_BC2: dst = ECHO_FMT_RGBA8_ETC2_EAC;	break;
			case ECHO_FMT_DDS_BC3: dst = ECHO_FMT_RGBA8_ETC2_EAC;	break;
			default:				dst = src;						break;
			}
		}

		return dst;
	}


	NameUnorderedSet Tex2DArrayInfo::setTexInvalid;

	void Tex2DArrayInfo::setLayerData(const Name& res_name,
		const MemoryDataStreamPtr& tex_stream)
	{
		EchoZoneScoped;

		tex_requests.erase(res_name);

		auto it = tex_index.find(res_name);
		if (it == tex_index.end())
			return;

		if (tex_stream.isNull()) {
			return;
		}

		texture->setTex2DArrayData(tex_stream->getPtr(), (uint32)tex_stream->size(), it->second, false);
	}

	int Tex2DArrayInfo::addTexture(const TexturePtr& ptrTex)
	{
		EchoZoneScoped;

		assert(!texture.isNull() && "The texture array is null");

		const int default_layer_id = depth - 1;
		if (ptrTex.isNull())
		{
			LogManager::instance()->logMessage("-error-\tThe texture is null, may be "
				"haven't load or not exist!", LML_CRITICAL);
			return default_layer_id;
		}
		const Name& texName = ptrTex->getName();

		if (ptrTex->isUseDefault())
		{
			if (setTexInvalid.find(texName) == setTexInvalid.end())
			{
				setTexInvalid.insert(texName);
				LogManager::instance()->logMessage("-error-\tThe texture isn't exist, "
					"use default texture! " + texName.toString(), LML_CRITICAL);
			}
			return default_layer_id;
		}

		if (ptrTex->isManual())
		{
			if (setTexInvalid.find(texName) == setTexInvalid.end())
			{
				setTexInvalid.insert(texName);
				LogManager::instance()->logMessage("-error-\tThe texture must be loaded "
					"from a file, not created manually! " + texName.toString(), LML_CRITICAL);
			}
			return default_layer_id;
		}

		if (ptrTex->getTextureType() != TEX_TYPE_2D)
		{
			if (setTexInvalid.find(texName) == setTexInvalid.end())
			{
				setTexInvalid.insert(texName);
				LogManager::instance()->logMessage("-error-\tThe texture must be an 2d texture: " +
					texName.toString(), LML_CRITICAL);
			}
			return default_layer_id;
		}

		// check the format and size
		const ECHO_TEX_DESC& pTex2DArrayDesc = texture->getTexDesc();
		const ECHO_TEX_DESC& pDesc = ptrTex->getTexDesc();
		if (pDesc.width != pTex2DArrayDesc.width ||
			pDesc.height != pTex2DArrayDesc.height ||
			pDesc.format != pTex2DArrayDesc.format)
		{
			if (setTexInvalid.find(texName) == setTexInvalid.end())
			{
				setTexInvalid.insert(texName);
				char szBuf[256] = { '\0' };
				snprintf(szBuf, sizeof(szBuf) - 1, "-error-\tTexture format doesn't match: %s, "
					"texture - dest(w: %d-%d, h: %d-%d, f:%d-%d)",
					texName.c_str(), pDesc.width, pTex2DArrayDesc.width,
					pDesc.height, pTex2DArrayDesc.height,
					(int)pDesc.format, (int)pTex2DArrayDesc.format);
				LogManager::instance()->logMessage(szBuf, LML_CRITICAL);
			}
			return default_layer_id;
		}

		auto it = tex_index.find(texName);
		if (it != tex_index.end())
			return it->second;

		if (layer_count >= depth)
		{
			if (setTexInvalid.find(texName) == setTexInvalid.end())
			{
				setTexInvalid.insert(texName);
				char szBuf[256] = { '\0' };
				snprintf(szBuf, sizeof(szBuf) - 1, "-error-\tTexture layer count exceed the limits: "
					"%s, %s %s %d %d %d", texName.c_str(), name.c_str(), tag.c_str(), width, height, depth);
				LogManager::instance()->logMessage(szBuf, LML_CRITICAL);
			}
			return default_layer_id;
		}

		// whether is loading
		if (tex_requests.find(texName) != tex_requests.end())
			return default_layer_id;

		int tex_layer = layer_count;
		tex_index[texName] = tex_layer;

		static bool synchronization = true;
		if (synchronization) {
			tex_requests[texName] = InstanceMaterialHelper::instance()->addStreamRequest(texName, this);
		}
		else
		{
			std::unique_ptr<char[]> tmp(new char[texName.size() + 5]);
			strcpy(tmp.get(), texName.c_str());
			_cat_tex_file_ext(tmp.get());

			MemoryDataStreamPtr texStream(static_cast<MemoryDataStream*>(Root::instance()->GetMemoryDataStream(tmp.get())));
			if (!texStream.isNull()) {
				setLayerData(texName, texStream);
			}
			else {
				LogManager::instance()->logMessage("-error-\tRead texture file failed: " +
					texName.toString(), LML_CRITICAL);
			}
		}

		++layer_count;
		return tex_layer;
	}

	void Tex2DArrayInfo::init()
	{
		EchoZoneScoped;

		assert(texture.isNull());
		assert(width > 0 && height > 0 && depth > 0);
		assert(width == height);

		ECHO_FMT format = ECHO_FMT_INVALID;
		Name init_layer, default_layer;

		const ECHO_TEX_TYPE renderTexType = Root::instance()->getRenderTextureType();
		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
		if (ECHO_DEVICE_DX9C == eClass || ECHO_DEVICE_DX11 == eClass)
		{
			format = ddsFormat;
			init_layer = dds_init_layer;
			default_layer = dds_default_layer;
		}
		else if (eClass == ECHO_DEVICE_GLES3 || eClass == ECHO_DEVICE_METAL)
		{
			if (ECHO_TEX_TYPE_DXT == renderTexType)
			{
				format = ddsFormat;
				init_layer = dds_init_layer;
				default_layer = dds_default_layer;
			}
			else if (ECHO_TEX_TYPE_ASTC == renderTexType)
			{
				format = astcFormat;
				init_layer = astc_init_layer;
				default_layer = astc_default_layer;
			}
			else if (ECHO_TEX_TYPE_ETC == renderTexType)
			{
				format = etcFormat;
				init_layer = etc_init_layer;
				default_layer = etc_default_layer;
			}
		}

		assert(format != ECHO_FMT_INVALID);

		char szBuff[128] = { '\0' };
		snprintf(szBuff, sizeof(szBuff) - 1, "%s/%s/TextureArray/%u/%u/%u",
			name.c_str(), tag.c_str(), width, height, depth);
		texture = TextureManager::instance()->createManual(
			Name(szBuff), TEX_TYPE_2D_ARRAY, width, height, depth,
			_mipmap_level(width), format, nullptr, 0);
		texture->load();

		if (!init_layer.empty())
		{
			MemoryDataStreamPtr texStream(static_cast<MemoryDataStream*>(
				Root::instance()->GetMemoryDataStream(init_layer.c_str())));
			if (!texStream.isNull())
			{
				//for (uint32 d=0u; d<depth; ++d) {
				texture->setTex2DArrayData(texStream->getPtr(), (uint32)texStream->size(), 0, depth, false);
				//}
			}
			else
			{
				std::string errmsg;
				errmsg.reserve(256);
				errmsg.append("[").append(name).append("]")
					.append(" -error-\tOpen init layer failed: ")
					.append(init_layer.c_str());
				LogManager::instance()->logMessage(LML_CRITICAL, errmsg);
			}
		}

		if (!default_layer.empty())
		{
			MemoryDataStreamPtr texStream(static_cast<MemoryDataStream*>(
				Root::instance()->GetMemoryDataStream(default_layer.c_str())));
			if (!texStream.isNull())
			{
				tex_index[default_layer] = depth - 1;
				setLayerData(default_layer, texStream);
			}
			else
			{
				std::string errmsg;
				errmsg.reserve(256);
				errmsg.append("[").append(name).append("]")
					.append(" -error-\tOpen default layer failed: ")
					.append(default_layer.c_str());
				LogManager::instance()->logMessage(LML_CRITICAL, errmsg);
			}
		}
	}

	void Tex2DArrayInfo::reset()
	{
		EchoZoneScoped;

		texture.setNull();
		tex_index.clear();
		for (auto& it : tex_requests)
			InstanceMaterialHelper::instance()->abortRequest(it.second);
		tex_requests.clear();
	}

	void Tex2DArrayInfo::refresh()
	{
		EchoZoneScoped;

		if (texture.isNull())
			return;

		Name init_layer, default_layer;
		const ECHO_TEX_TYPE renderTexType = Root::instance()->getRenderTextureType();
		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
		if (ECHO_DEVICE_DX9C == eClass || ECHO_DEVICE_DX11 == eClass) {
			init_layer = dds_init_layer;
			default_layer = dds_default_layer;
		}
		else if (eClass == ECHO_DEVICE_GLES3 || eClass == ECHO_DEVICE_METAL)
		{
			if (ECHO_TEX_TYPE_DXT == renderTexType) {
				init_layer = dds_init_layer;
				default_layer = dds_default_layer;
			}
			else if (ECHO_TEX_TYPE_ASTC == renderTexType) {
				init_layer = astc_init_layer;
				default_layer = astc_default_layer;
			}
			else if (ECHO_TEX_TYPE_ETC == renderTexType) {
				init_layer = etc_init_layer;
				default_layer = etc_default_layer;
			}
		}

		if (!init_layer.empty())
		{
			MemoryDataStreamPtr texStream(static_cast<MemoryDataStream*>(
				Root::instance()->GetMemoryDataStream(init_layer.c_str())));
			if (!texStream.isNull()) {
				texture->setTex2DArrayData(texStream->getPtr(), (uint32)texStream->size(), 0, depth, false);
			}
			else
			{
				std::string errmsg;
				errmsg.reserve(256);
				errmsg.append("[").append(name).append("]")
					.append(" -error-\tOpen init layer failed: ")
					.append(init_layer.c_str());
				LogManager::instance()->logMessage(LML_CRITICAL, errmsg);
			}
		}

		for (const auto& tex : tex_index)
		{
			MemoryDataStreamPtr texStream(static_cast<MemoryDataStream*>(
				Root::instance()->GetMemoryDataStream(tex.first.c_str())));
			if (!texStream.isNull())
			{
				bool bValidTex = true;
				TexturePtr ptrTex = TextureManager::instance()->loadTexture(tex.first);
				if (bValidTex && ptrTex->isManual())
				{
					LogManager::instance()->logMessage("-error-\tThe texture must be loaded "
						"from a file, not created manually! " + tex.first.toString(), LML_CRITICAL);
					bValidTex = false;
				}

				if (bValidTex && ptrTex->getTextureType() != TEX_TYPE_2D)
				{
					LogManager::instance()->logMessage("-error-\tThe texture must be loaded "
						"from a file, not created manually! " + tex.first.toString(), LML_CRITICAL);
					bValidTex = false;
				}

				if (bValidTex)
				{
					const ECHO_TEX_DESC& pTex2DArrayDesc = texture->getTexDesc();
					const ECHO_TEX_DESC& pDesc = ptrTex->getTexDesc();
					if (pDesc.width != pTex2DArrayDesc.width ||
						pDesc.height != pTex2DArrayDesc.height ||
						pDesc.format != pTex2DArrayDesc.format)
					{
						char szBuf[256] = { '\0' };
						snprintf(szBuf, sizeof(szBuf) - 1, "-error-\tTexture format doesn't match: %s, "
							"texture - dest(w: %d-%d, h: %d-%d, f:%d-%d)",
							tex.first.c_str(), pDesc.width, pTex2DArrayDesc.width,
							pDesc.height, pTex2DArrayDesc.height,
							(int)pDesc.format, (int)pTex2DArrayDesc.format);
						LogManager::instance()->logMessage(szBuf, LML_CRITICAL);
						bValidTex = false;
					}
				}

				if (bValidTex) {
					setLayerData(tex.first, texStream);
				}
				else
				{
					MemoryDataStreamPtr default_layer_stream(static_cast<MemoryDataStream*>(
						Root::instance()->GetMemoryDataStream(default_layer.c_str())));
					if (!default_layer_stream.isNull()) {
						setLayerData(tex.first, default_layer_stream);
					}
					else {
						std::string errmsg;
						errmsg.reserve(256);
						errmsg.append("[").append(name).append("]")
							.append(" -error-\tOpen default layer failed: ")
							.append(default_layer.c_str());
						LogManager::instance()->logMessage(LML_CRITICAL, errmsg);
					}
				}
			}
			else
			{
				std::string errmsg;
				errmsg.reserve(256);
				errmsg.append("[").append(name).append("]")
					.append(" -error-\tOpen texture layer failed: ")
					.append(tex.first.c_str());
				LogManager::instance()->logMessage(LML_CRITICAL, errmsg);
			}
		}
	}

	using InstanceDataUpdateFunc = void (*)(const SubEntityVec&, uint32, Uniformf4Vec&);

	/** 某种可以合批的 shader 的一些信息，包括一个批次可以支持
		多少个，instance data 的组织方式等等
	*/
	struct InstanceTypeInfo
	{
		InstanceType type = eIT_None;
		// 每个实例的按照 4 个 float（float4 in HLSL, vec4 in OpenGL ES SL）
		// 组织后占用的 uniform 的个数。
		int instance_uniform_count = 0;

		// 保留的 uniform 数量，shader 用来传递一些全局或材质共用的参数
		int rev_uniform_count = 0;

		InstanceDataUpdateFunc func = nullptr;
	};

	using InstanceTypeInfoMap = std::unordered_map<int, InstanceTypeInfo>;
	using TypeShaderMap = std::unordered_map<int, String>;
	using ShaderTypeMap = std::unordered_map<String, int>;

	class InstanceBatchEntityRender;
	using InstanceBatchEntityRenderVec = std::vector<InstanceBatchEntityRender*>;

	//--------------------------------------------------------------------------
	// InstanceBatchEntityPrivate
	class InstanceBatchEntityPrivate : public EchoAllocatedObject
	{
	public:
		InstanceTypeInfo mInfo;

		SubEntityVec mInstances;
		Uniformf4Vec mInstanceData;

		InstanceBatchEntityRenderVec mRenderPool;
		RenderOperation mRenderOperation;
		IndexBuffInfo mIndexBuffInfo;

		// The count of instance can be drawed in per batch
		int mInstanceCount = 0;
		std::vector<ShaderMacrosItem> mInstanceMacros;
		MaterialWrapper mMaterial;
		WorkQueue::RequestID mMatTick = 0;

		bool initCaps()
		{
			EchoZoneScoped;

			if (eIT_None == mInfo.type)
				return false;

			uint64_t iMaxVtxUniformCount = Root::instance()->getRenderSystem()->
				getDependentLimit(ECHO_MAX_VERTEX_UNIFORM_VECTORS_NUMBER);
			if (0 == iMaxVtxUniformCount)
				return false;

			mInstanceCount = int(std::min(iMaxVtxUniformCount - mInfo.rev_uniform_count, uint64_t(std::numeric_limits<int>::max())));
			mInstanceCount /= mInfo.instance_uniform_count;
			if (mInstanceCount <= 0)
				return false;

			mInstanceMacros.resize(2);
			mInstanceMacros[0] = ShaderMacrosItem("InstanceCount", mInstanceCount);
			mInstanceMacros[1] = ShaderMacrosItem("UniformCount", mInfo.instance_uniform_count);

			return true;
		}

		static LightList mLightList;

		static struct UtilType
		{
			InstanceTypeInfoMap mInstTypeInfoMap;
			TypeShaderMap mTypeShaderMap;
			ShaderTypeMap mShaderTypeMap;

			///	In DirectX3D 9, there isn't an 'InstanceID' like OpenGL ES 3.0
			/// when draw an object in instance, so we use an extra vertex buffer
			/// as the 'InstanceID'.
			RcBuffer* mBuffInstID = nullptr;
			static const uint32 MaxInstancePerBatch = 256;

			InstanceType getInstanceType(const String& sShader) const
			{
				EchoZoneScoped;

				auto it = mShaderTypeMap.find(sShader);
				if (it != mShaderTypeMap.end())
					return (InstanceType)it->second;
				return eIT_None;
			}

			InstanceTypeInfo getInstanceInfo(const String& sShader) const
			{
				EchoZoneScoped;

				return getInstanceInfo(getInstanceType(sShader));
			}

			InstanceTypeInfo getInstanceInfo(InstanceType type) const
			{
				EchoZoneScoped;

				auto it = mInstTypeInfoMap.find(type);
				if (it != mInstTypeInfoMap.end())
					return it->second;

				return InstanceTypeInfo();
			}

		} Util;
	}; //class InstanceBatchEntityPrivate

	using IBEPri = InstanceBatchEntityPrivate;
	LightList IBEPri::mLightList;
	IBEPri::UtilType IBEPri::Util;

	// InstanceBatchEntityPrivate
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// InstanceBatchEntityRender
	class InstanceBatchEntityRender : public Renderable
	{
	public:
		InstanceBatchEntityRender()
		{
			EchoZoneScoped;

			enableCustomParameters(true);
			enableInstanceRender(true);
		}

		virtual void getWorldTransforms(const DBMatrix4** xform) const override
		{
			EchoZoneScoped;

			if (mMirrored)
			{
				static const DBMatrix4 mirrored_mat =
				{
					-1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				};
				*xform = &mirrored_mat;
			}
		}

		virtual RenderOperation* getRenderOperation() override
		{
			EchoZoneScoped;
			return &mBatch->dp->mRenderOperation;
		}

		virtual const Material* getMaterial() const override
		{
			EchoZoneScoped;
			return mBatch->dp->mMaterial.getV1();
		}

		const MaterialWrapper& getMaterialWrapper() const override
		{
			EchoZoneScoped;
			return mBatch->dp->mMaterial;
		}

		virtual const LightList& getLights() const override
		{
			EchoZoneScoped;
			return InstanceBatchEntity::GetLightList();
		}

		virtual uint32 getInstanceNum() override
		{
			EchoZoneScoped;
			return mInstNum;
		}

		virtual void setCustomParameters(const Pass* pPass) override
		{
			EchoZoneScoped;
			mBatch->updatePassParam(this, pPass, mInstIdxStart, mInstNum);
		}

		virtual void getIndexBuffInfo(IndexBuffInfo& indexInfo) override
		{
			EchoZoneScoped;
			indexInfo = mBatch->dp->mIndexBuffInfo;
		}

		uint32 mInstIdxStart = 0u;
		uint32 mInstNum = 0u;
		InstanceBatchEntity* mBatch = nullptr;
		bool mMirrored = false;
	}; // class InstanceBatchEntityRender
	// InstanceBatchEntityRender
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// InstanceBatchEntity
	InstanceBatchEntity::InstanceBatchEntity(const InstanceKey& key)
		: mKey(key)
		, dp(new InstanceBatchEntityPrivate)
	{
		EchoZoneScoped;

		dp->mInstances.reserve(32);
		dp->mRenderPool.reserve(4);
	}

	InstanceBatchEntity::~InstanceBatchEntity()
	{
		EchoZoneScoped;

		if (dp->mMatTick != 0)
			InstanceMaterialHelper::instance()->abortRequest(dp->mMatTick);
		dp->mMatTick = 0;
		//for (int i=LOD_LEVEL_0; i<LOD_LEVEL_NUM; ++i)
		{
			auto& data = dp->mRenderOperation.m_stMeshData;
			data.m_vertexBuffer.clear();
			data.m_vertexCount = data.m_vertexBuffSize = 0;
			data.m_IndexBufferData.m_indexBuffer = nullptr;
			data.m_IndexBufferData.m_indexCount = data.m_indexBufferSize = 0;
		}
		for (const auto i : dp->mRenderPool)
			delete i;
		delete dp;	dp = nullptr;
	}

	InstanceKey InstanceBatchEntity::GenBatchKey(SubEntity* pSubEntity)
	{
		EchoZoneScoped;

		SubMesh* pSubMesh = pSubEntity->getSubMesh();
		const Name& nmMesh = pSubMesh->mParent->getName();
		uint64 nmMeshHashVal = nmMesh.getHashVal();
		uint64 idxSubMesh = pSubMesh->mIndex;
		uint64 idxBuf = pSubEntity->getBuffIndex();

		InstanceKey key;
		const MaterialWrapper& mat = pSubEntity->getMaterialWrapper();
		if (mat.isNull())
		{
			return key;
		}
		uint64 nmMatHashVal = mat.getName().getHashVal();
		key.v[0] = (nmMeshHashVal & 0xFFFFFFFF) | (idxSubMesh << 32);
		key.v[1] = (idxBuf & 0xFFFFFFFF) | (nmMatHashVal << 32);
		return key;
	}

	bool InstanceBatchEntity::init(SubEntity* pSubEntity)
	{
		EchoZoneScoped;

		MaterialWrapper baseMat;
		if (pSubEntity != nullptr)
			baseMat = pSubEntity->getRealMaterialWrapper();
		if (baseMat.isNull())
			return false;

		const String shaderName = baseMat.isV1() ? baseMat.getV1()->m_matInfo.shadername : baseMat.getV2()->getShaderPassName().toString();

		dp->mInfo = IBEPri::Util.getInstanceInfo(shaderName);
		if (eIT_None == dp->mInfo.type)
			return false;

		SubMesh* pSubMesh = pSubEntity->getSubMesh();
		RenderOperation* pOp = pSubMesh->_getRenderOperation();
		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
		//for (int i=LOD_LEVEL_0; i<LOD_LEVEL_NUM; ++i)
		{
			if (!pOp->m_stMeshData.m_vertexBuffer.empty() && ECHO_DEVICE_DX9C == eClass)
			{
				dp->mRenderOperation.m_stMeshData.m_vertexBuffer.reserve(
					pOp->m_stMeshData.m_vertexBuffer.size() + 1);
			}
			dp->mRenderOperation.m_stMeshData = pOp->m_stMeshData;
			if (!pOp->m_stMeshData.m_vertexBuffer.empty() && ECHO_DEVICE_DX9C == eClass) {
				dp->mRenderOperation.m_stMeshData.m_vertexBuffer.push_back(IBEPri::Util.mBuffInstID);
			}
		}
		// dp->mRenderOperation = pSubMesh->_getRenderOperation();
		dp->mRenderOperation.mPrimitiveType = pOp->mPrimitiveType;
		pSubEntity->getIndexBuffInfo(dp->mIndexBuffInfo);

		if (!dp->initCaps())
			return false;

		// create batch material, include point light, shader resive
		SceneManager* pSceneMgr = pSubEntity->getParent()->_getManager();
		bool bShadowEnable = pSceneMgr->getPsmshadowManager()->getShadowEnable();
		initMaterial(baseMat, bShadowEnable, pSceneMgr->getPointShadowManager()->IsEnabled());

		return true;
	}

	void InstanceBatchEntity::initMaterial(const MaterialWrapper& baseMat, bool bShadowEnable, bool pointShadow)
	{
		EchoZoneScoped;

		if (baseMat.isV2())
		{
			MaterialV2Ptr ptrMat = MaterialV2Manager::instance()->createManual();

			ptrMat->cloneFrom(baseMat.getV2(), false);
			ptrMat->setBasePassMaskFlags(PassMaskFlags(PassMaskFlag::UseInstance));
			ptrMat->setCustomVSMacros(dp->mInstanceMacros);
			ptrMat->prepareShaderPass();

			dp->mMatTick = InstanceMaterialHelper::instance()->addEntityMatReq(this, ptrMat);
		}
		else
		{
			MaterialPtr ptrMat = MaterialManager::instance()->createManual();

			ptrMat->m_desc = baseMat.getV1()->m_desc;
			ptrMat->m_matInfo = baseMat.getV1()->m_matInfo;

			if (Root::instance()->getDeviceType() == ECHO_DEVICE_DX9C)
			{
				// Add an extra instance id buffer
				uint32 idxInst = ptrMat->m_desc.IADesc.nStreamCount;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].bPreInstance = ECHO_TRUE;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].nRate = 1;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].nStride = 4;
				++ptrMat->m_desc.IADesc.nStreamCount;

				uint32 idxInstElem = ptrMat->m_desc.IADesc.nElementCount;
				ECHO_IA_ELEMENT& refElem = ptrMat->m_desc.IADesc.ElementArray[idxInstElem];
				refElem.nStream = idxInst;
				refElem.nIndex = 0u;
				refElem.eType = ECHO_UBYTE4;
				refElem.nOffset = 0u;
				refElem.Semantics = ECHO_BLENDINDICES1;
				++ptrMat->m_desc.IADesc.nElementCount;
			}

			String strVS, strPS, strVSMacro, strPSMacro;
			strVSMacro.reserve(128);
			strPSMacro.reserve(128);
			MaterialManager::instance()->getShader(ptrMat.get(), strVS, strPS,
				strVSMacro, strPSMacro);
			for (const auto& i : dp->mInstanceMacros)
			{
				strVSMacro.append(i.toString());
			}
			ptrMat->setShaderSrcCode(strVS, strPS);
			ptrMat->setMacroShaderSrcCode(strVSMacro, strPSMacro);
			ptrMat->createPass(InstancePass);
			ptrMat->createPass(InstanceLightPass);

			ptrMat->createPass(InstanceRipplePass);
			ptrMat->createPass(InstanceLightRipplePass);
			//if (ptrMat->m_matInfo.bSupportSoft)
			//{
			//	ptrMat->createPass(InstanceSoftPass);
			//	ptrMat->createPass(InstanceLightSoftPass);
			//}
			if (bShadowEnable)
			{
				//if (ptrMat->m_matInfo.bSupportSoft)
				//{
				//	ptrMat->createPass(ShadowResiveInstanceSoftPass);
				//	ptrMat->createPass(ShadowResiveInstanceLightSoftPass);
				//}
				ptrMat->createPass(ShadowResiveInstancePass);
				ptrMat->createPass(ShadowResiveInstanceLightPass);
				ptrMat->createPass(ShadowResiveInstanceRipplePass);
				ptrMat->createPass(ShadowResiveInstanceLightRipplePass);
				ptrMat->createPass(ShadowCasterInstancePass);
			}
			if (pointShadow)
			{
				ptrMat->createPass(PointShadowCasterInstance);
				ptrMat->createPass(PointShadowReceiverInstance);
				ptrMat->createPass(PointShadowReceiverRippleInstance);
				//ptrMat->createPass(PointShadowReceiverSoftInstance);
				//ptrMat->createPass(PointShadowReceiverSoftRippleInstance);
			}
			if (bShadowEnable && pointShadow)
			{
				ptrMat->createPass(ShadowReceiverPointShadowReceiverInstance);
				ptrMat->createPass(ShadowReceiverPointShadowReceiverRippleInstance);
				//ptrMat->createPass(ShadowReceiverPointShadowReceiverSoftInstance);
				//ptrMat->createPass(ShadowReceiverPointShadowReceiverSoftRippleInstance);
			}

			ptrMat->setMaterialPassMask(1ull << MPO_ShadowResiveInstancePass | 1ull << MPO_ShadowResiveInstanceLightPass |
				1ull << MPO_ShadowResiveInstanceRipplePass | 1ull << MPO_ShadowResiveInstanceLightRipplePass);

			ptrMat->setMaterialRipplePassMask(1ull << MRPO_InstanceRipplePass | 1ull << MRPO_InstanceLightRipplePass |
				1ull << MRPO_ShadowResiveInstanceRipplePass | 1ull << MRPO_ShadowResiveInstanceLightRipplePass);

			ptrMat->setMaterialPointShadowPassMask(
				MPSPO_PointShadowCasterInstance,
				MPSPO_PointShadowReceiverInstance,
				MPSPO_PointShadowReceiverRippleInstance,
				MPSPO_ShadowReceiverPointShadowReceiverInstance,
				MPSPO_ShadowReceiverPointShadowReceiverRippleInstance
				//MPSPO_PointShadowReceiverSoftInstance,
				//MPSPO_PointShadowReceiverSoftRippleInstance,
				//MPSPO_ShadowReceiverPointShadowReceiverSoftInstance,
				//MPSPO_ShadowReceiverPointShadowReceiverSoftRippleInstance
			);
			dp->mMatTick = InstanceMaterialHelper::instance()->addEntityMatReq(this, ptrMat);
		}
	}

	void InstanceBatchEntity::materialLoaded(const MaterialWrapper& mat, uint64 requestID)
	{
		EchoZoneScoped;

		dp->mMaterial = mat;
		dp->mMatTick = 0;
	}

	MaterialWrapper InstanceBatchEntity::getMaterial() const
	{
		EchoZoneScoped;

		return dp->mMaterial;
	}

	bool InstanceBatchEntity::addInstance(SubEntity* pSubEntity)
	{
		EchoZoneScoped;

		if (nullptr == pSubEntity)
			return false;
#ifdef _DEBUG
		auto it = std::find(dp->mInstances.begin(), dp->mInstances.end(), pSubEntity);
		if (it != dp->mInstances.end())
		{
			assert(false && "The instance has been added!");
			return false;
		}
#endif
		if (dp->mMaterial.isNull())
			return false;
		dp->mInstances.push_back(pSubEntity);
		return true;
	}

	void InstanceBatchEntity::clearInstance()
	{
		EchoZoneScoped;

		dp->mInstances.clear();
		dp->mInstanceData.clear();
	}

	bool InstanceBatchEntity::hasInstance() const
	{
		EchoZoneScoped;

		return !dp->mInstances.empty();
	}

	namespace
	{
		uint32 _sort_mirrored(SubEntityVec& instances)
		{
			EchoZoneScoped;

			int inst_num = (int)instances.size();
			int b = 0, e = (int)inst_num - 1;
			while (b <= e)
			{
				//const DBMatrix4 * _world_matrix_b = nullptr;
				//const DBMatrix4* _world_matrix_e = nullptr;
				//instances[b]->getWorldTransforms(&_world_matrix_b);
				//instances[e]->getWorldTransforms(&_world_matrix_e);

				//Matrix3 _world_matrix_b_3x3, _world_matrix_e_3x3;
				//_world_matrix_b->extract3x3Matrix(_world_matrix_b_3x3);
				//_world_matrix_e->extract3x3Matrix(_world_matrix_e_3x3);

				//bool mirrored_b = _world_matrix_b_3x3.Determinant() < 0;
				//bool mirrored_e = _world_matrix_e_3x3.Determinant() < 0;

				bool mirrored_b = instances[b]->getMirrored();
				bool mirrored_e = instances[e]->getMirrored();
				if (mirrored_b < mirrored_e)
				{
					std::swap(instances[b], instances[e]);
					++b;	--e;
				}
				else
				{
					if (mirrored_b)	++b;
					else			--e;
				}
			}
			return b;
		}
	}

	std::tuple<uint32, uint32> InstanceBatchEntity::updateRenderQueue(PassType typePass, RenderQueue* pRenderQueue)
	{
		EchoZoneScoped;

		std::tuple<uint32, uint32> tpNum(0u, 0u);
		if (!dp->mMaterial.isV1())
			return tpNum;

		Pass* pPass = dp->mMaterial.getV1()->getPass(typePass);
		if (nullptr == pPass)
			return tpNum;

		int mirrored_num = _sort_mirrored(dp->mInstances);
		// update instance data by instance type
		if (dp->mInfo.func != nullptr)
			dp->mInfo.func(dp->mInstances, dp->mInfo.instance_uniform_count, dp->mInstanceData);
		// add render object to render queue
		RenderQueueGroup* pGroup = nullptr;
		if (dp->mMaterial.getV1()->m_matInfo.alphaTestEnable)
			pGroup = pRenderQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest);
		else
			pGroup = pRenderQueue->getRenderQueueGroup(RenderQueue_Solid);

		uint32 iRenderIdx = 0;
		for (int i = 0; i < mirrored_num; i += dp->mInstanceCount)
		{
			if (iRenderIdx >= dp->mRenderPool.size())
				dp->mRenderPool.push_back(new InstanceBatchEntityRender);
			InstanceBatchEntityRender* pRender = dp->mRenderPool[iRenderIdx];
			pRender->mInstIdxStart = i;
			pRender->mInstNum = dp->mInstanceCount;
			pRender->mBatch = this;
			pRender->mMirrored = true;
			if (pRender->mInstIdxStart + pRender->mInstNum >= (uint32)mirrored_num)
				pRender->mInstNum = mirrored_num - pRender->mInstIdxStart;
			pGroup->addRenderable(pPass, pRender);
			++iRenderIdx;
		}

		int total_num = (int)dp->mInstances.size();
		for (int i = mirrored_num; i < total_num; i += dp->mInstanceCount)
		{
			if (iRenderIdx >= dp->mRenderPool.size())
				dp->mRenderPool.push_back(new InstanceBatchEntityRender);
			InstanceBatchEntityRender* pRender = dp->mRenderPool[iRenderIdx];
			pRender->mInstIdxStart = i;
			pRender->mInstNum = dp->mInstanceCount;
			pRender->mBatch = this;
			pRender->mMirrored = false;
			if (pRender->mInstIdxStart + pRender->mInstNum >= (uint32)total_num)
				pRender->mInstNum = total_num - pRender->mInstIdxStart;
			pGroup->addRenderable(pPass, pRender);
			++iRenderIdx;
		}

		Root::instance()->addBatchDrawCount((uint32)total_num - iRenderIdx);
		return std::tuple<uint32, uint32>((uint32)total_num, iRenderIdx);
	}

	std::tuple<uint32, uint32> InstanceBatchEntity::updateRenderQueueV2(bool shadowPass, PassMaskFlags passMaskFlags, RenderQueue* pRenderQueue)
	{
		EchoZoneScoped;

		std::tuple<uint32, uint32> tpNum(0u, 0u);
		if (!dp->mMaterial.isV2())
			return tpNum;

		MaterialV2* mat = dp->mMaterial.getV2();

		int mirrored_num = _sort_mirrored(dp->mInstances);
		// update instance data by instance type
		if (dp->mInfo.func != nullptr)
			dp->mInfo.func(dp->mInstances, dp->mInfo.instance_uniform_count, dp->mInstanceData);
		// add render object to render queue

		uint32 iRenderIdx = 0;
		for (int i = 0; i < mirrored_num; i += dp->mInstanceCount)
		{
			if (iRenderIdx >= dp->mRenderPool.size())
				dp->mRenderPool.push_back(new InstanceBatchEntityRender);
			InstanceBatchEntityRender* pRender = dp->mRenderPool[iRenderIdx];
			pRender->mInstIdxStart = i;
			pRender->mInstNum = dp->mInstanceCount;
			pRender->mBatch = this;
			pRender->mMirrored = true;
			if (pRender->mInstIdxStart + pRender->mInstNum >= (uint32)mirrored_num)
				pRender->mInstNum = mirrored_num - pRender->mInstIdxStart;

			PostRenderableParams postParams{
				.material = mat,
				.renderable = pRender,
				.renderQueue = pRenderQueue,
				.passMaskFlags = passMaskFlags,
				.basePriority = DEFAULT_PRIORITY,
				.isShadowPass = shadowPass
			};
			MaterialShaderPassManager::fastPostRenderable(postParams);

			++iRenderIdx;
		}

		int total_num = (int)dp->mInstances.size();
		for (int i = mirrored_num; i < total_num; i += dp->mInstanceCount)
		{
			if (iRenderIdx >= dp->mRenderPool.size())
				dp->mRenderPool.push_back(new InstanceBatchEntityRender);
			InstanceBatchEntityRender* pRender = dp->mRenderPool[iRenderIdx];
			pRender->mInstIdxStart = i;
			pRender->mInstNum = dp->mInstanceCount;
			pRender->mBatch = this;
			pRender->mMirrored = false;
			if (pRender->mInstIdxStart + pRender->mInstNum >= (uint32)total_num)
				pRender->mInstNum = total_num - pRender->mInstIdxStart;

			PostRenderableParams postParams{
				.material = mat,
				.renderable = pRender,
				.renderQueue = pRenderQueue,
				.passMaskFlags = passMaskFlags,
				.basePriority = DEFAULT_PRIORITY,
				.isShadowPass = shadowPass
			};
			MaterialShaderPassManager::fastPostRenderable(postParams);

			++iRenderIdx;
		}

		Root::instance()->addBatchDrawCount((uint32)total_num - iRenderIdx);

		return std::make_tuple((uint32)total_num, iRenderIdx);
	}

	void InstanceBatchEntity::updatePassParam(const Renderable* pRend,
		const Pass* pPass, uint32 iInstIdxStart, uint32 iInstNum)
	{
		EchoZoneScoped;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		pRenderSystem->setUniformValue(pRend, pPass, U_VSCustom0,
			dp->mInstanceData.data() + iInstIdxStart * dp->mInfo.instance_uniform_count,
			iInstNum * dp->mInfo.instance_uniform_count * sizeof(dp->mInstanceData[0]));
	}


	void InstanceBatchEntity::SetLightList(const LightList& lights)
	{
		EchoZoneScoped;

		IBEPri::mLightList = lights;
	}

	const LightList& InstanceBatchEntity::GetLightList(void)
	{
		EchoZoneScoped;

		return IBEPri::mLightList;
	}

	void InstanceBatchEntity::Init()
	{
		EchoZoneScoped;

		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();

		auto _l_register_shader = [](IBEPri::UtilType& util, InstanceType type, const char* shader,
			int inst_uniform_count, int rev_uniform_count, InstanceDataUpdateFunc _func)
		{
			InstanceTypeInfo& info = util.mInstTypeInfoMap[type];
			info.type = type;
			info.instance_uniform_count = inst_uniform_count;
			info.rev_uniform_count = rev_uniform_count;
			info.func = _func;
			util.mTypeShaderMap[type] = shader;
			util.mShaderTypeMap[shader] = type;
		};

#define shader_type_register(util, type, inst_uni_count, rev_uni_count, _func) \
		_l_register_shader(util, eIT_##type, #type, (inst_uni_count), (rev_uni_count), _func);

		shader_type_register(IBEPri::Util, Illum, 6, 58, updateWorldTransform);
		shader_type_register(IBEPri::Util, IllumPBR, 6, 58, updateWorldTransform);
		shader_type_register(IBEPri::Util, IllumPBR2022, 6, 58, updateWorldTransform);
		shader_type_register(IBEPri::Util, IllumPBR2023, 6, 58, updateWorldTransform);
		shader_type_register(IBEPri::Util, SpecialIllumPBR, 6, 58, updateWorldTransform);
		shader_type_register(IBEPri::Util, BillboardLeaf, 3, 58, updateWorldPositionScaleOrientationOri);
		shader_type_register(IBEPri::Util, BillboardLeafPBR, 3, 58, updateWorldPositionScaleOrientationOri);
		shader_type_register(IBEPri::Util, Trunk, 3, 58, updateWorldPositionScaleOrientation);
		shader_type_register(IBEPri::Util, TrunkPBR, 3, 58, updateWorldPositionScaleOrientation);
		shader_type_register(IBEPri::Util, SimpleTreePBR, 6, 58, updateWorldTransform);
		shader_type_register(IBEPri::Util, BaseWhiteNoLight, 3, 58, updateWorldPositionScaleOrientation);
#undef shader_type_register

		if (eClass == ECHO_DEVICE_DX9C)
		{
			uint8 iInstID[IBEPri::UtilType::MaxInstancePerBatch * 4] = { 0u, };
			for (uint32 i = 0; i < IBEPri::UtilType::MaxInstancePerBatch; ++i)
			{
				iInstID[i * 4 + 0] = (uint8)i;
				iInstID[i * 4 + 1] = 0;
				iInstID[i * 4 + 2] = 0;
				iInstID[i * 4 + 3] = 0;
			}
			IBEPri::Util.mBuffInstID = Root::instance()->getRenderSystem()->
				createStaticVB(iInstID, IBEPri::UtilType::MaxInstancePerBatch * 4);
		}
	}

	void InstanceBatchEntity::Destroy()
	{
		EchoZoneScoped;

		if (IBEPri::Util.mBuffInstID != nullptr)
		{
			Root::instance()->getRenderSystem()->destoryBuffer(IBEPri::Util.mBuffInstID);
			IBEPri::Util.mBuffInstID = nullptr;
		}
	}

	bool InstanceBatchEntity::IsShaderSupported(const String& sShader)
	{
		EchoZoneScoped;

		return IBEPri::Util.mShaderTypeMap.find(sShader) != IBEPri::Util.mShaderTypeMap.end();
	}
	// InstnaceBatchEntity 
	//--------------------------------------------------------------------------

} // namespace Echo
