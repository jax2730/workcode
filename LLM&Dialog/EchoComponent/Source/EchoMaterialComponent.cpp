#include "EchoComponentStableHeaders.h"
#include "EchoMaterialComponent.h"
#include "EchoName.h"
//#include "tinyxml.h"
#include "EchoWorldNode.h"
#include "EchoSceneManager.h"
#include "EchoSceneNode.h"
#include "Actor/EchoActor.h"

#include "EchoActorProFuncImp.h"

#include "EchoEntity.h"
#include "EchoSubMesh.h"

#include "EchoMaterial.h"
#include "EchoShaderBin.h"
#include "EchoShaderMan.h"
#include "EchoLogManager.h"


namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(MaterialComponent)

	EDITOR_REFLECT_BEGIN(MaterialComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CurrentEntityName, "CurrentEntityName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CurrentSubEntityName, "CurrentSubEntityName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EntityComName, "EntityComName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RoleComName, "RoleComName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(m_Value, "Value", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	MaterialComponentInfo::MaterialComponentInfo(const Name& comType, ActorCOMMoveableType MoveType): ActorComponentInfo(comType, MoveType)
	{
	}

	MaterialComponentInfo::~MaterialComponentInfo()
	{
	}

	void MaterialComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<MaterialComponentInfo>::get()->ExportJson(this, jsonNode, "");
		if (IsCustomParam("m_Value"))
		{
			cJSON* item = cJSON_CreateArray();
			cJSON_AddItemToObject(jsonNode, "item", item);
			for (auto& it : m_ValueMap)
			{
				if (IsCustomParam(it.first))
				{
					cJSON* subEntity = cJSON_CreateObject();
					cJSON_AddItemToObject(item, "subEntity", subEntity);

					cJSON_AddStringToObject(subEntity, "subEntityName", it.first.c_str());
					cJSON* valueVec = cJSON_CreateArray();
					cJSON_AddItemToObject(subEntity, "valueVec", valueVec);
					for (int i = 0; i < it.second.size(); ++i)
					{
						cJSON_AddNumberToObject(valueVec, "", it.second[i]);
					}
				}
			}
		}
	}

	void MaterialComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<MaterialComponentInfo>::get()->ImportJson(this, jsonNode, "");
		auto json = Echo::cJSON_GetObjectItem(jsonNode, "item");
		if (json)
		{
			addCustomParam("m_Value", false);
			int subEntityCount = cJSON_GetArraySize(json);
			for (int i = 0; i < subEntityCount; ++i)
			{
				auto subEntity = json->child;
				auto subEntityName =  cJSON_GetObjectItem(subEntity, "subEntityName");
				auto valueVec = cJSON_GetObjectItem(subEntity, "valueVec");

				int valueCount = cJSON_GetArraySize(valueVec);
				for (int j = 0; j < valueCount; ++j)
				{
					auto value = cJSON_GetArrayItem(valueVec, j);
					m_ValueMap[subEntityName->valuestring].push_back((float)value->valuedouble);
				}
			}
		}
	}

	void MaterialComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		MaterialComponentInfo* info = dynamic_cast<MaterialComponentInfo*>(comInfo);
		reflect::TypeResolver<MaterialComponentInfo>::get()->copyTo(this, info);
		info->m_ValueMap = m_ValueMap;
	}

	void MaterialComponentInfo::Overlay(ActorComponentInfo* comInfo)
	{
		/*if (comInfo == nullptr)
			return;
		if (comInfo->m_Type != m_Type)
			return;

		MaterialComponentInfo* pinfo = static_cast<MaterialComponentInfo*>(comInfo);
		OVERLAY_PROP(this, pinfo, CurrentEntityName);
		OVERLAY_PROP(this, pinfo, CurrentSubEntityName);
		OVERLAY_PROP(this, pinfo, EntityComName);
		OVERLAY_PROP(this, pinfo, RoleComName);*/
	}

	void MaterialComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			MaterialComponentInfo* oriInfo = static_cast<MaterialComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<MaterialComponentInfo>::get()->overWrite(oriInfo, this);
			if (!IsCustomParam("m_Value") && !IsRealCustomParam("m_Value"))
			{
				m_ValueMap = oriInfo->m_ValueMap;
			}
		}
	}

	MaterialComponent::MaterialComponent(ActorPtr  actor, ActorCOMMoveableType type) :ActorComponent(actor, type)
		, m_entityCom(nullptr)
		, m_RoleCom(nullptr)
		, m_isLoaded(false)
		, m_entity(nullptr)
		, m_subEntity(nullptr)
	{
		//this->mType = COMPONENTTYPE(MaterialComponent);
	}

	MaterialComponent::~MaterialComponent()
	{
		
	}

	void MaterialComponent::init(ActorComponentInfo * info)
	{
		InitalizeChineseWords();
	}

	void MaterialComponent::bindNode()
	{

	}

	void MaterialComponent::connectCom()
	{
	}

	void MaterialComponent::replay(ActorComponentInfo * info)
	{
	}

	void MaterialComponent::copyTo(const ActorComponentPtr&  actCom)
	{
	}

	void MaterialComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		const std::string& name = name_.c_str();
		auto strIdx = name.find("##");
		if (strIdx != std::string::npos)
			strIdx += 2;
		std::string resIdx = std::string(name.begin() + strIdx, name.end());
	
		int idx = std::atoi(resIdx.data());

		auto tempMessage = FindMessageFromParamIndex(idx);

		if (tempMessage.paramIndex == -1)
			return;
		if (tempMessage.iscolor)
		{
			tempMessage.color.x = data[0];
			tempMessage.color.y = data[1];
			tempMessage.color.z = data[2];
			tempMessage.color.w = data[3];
		}
		else
		{
			tempMessage.val = data[0];
		}
	    SetMaterialParam(&tempMessage);
	}


	void MaterialComponent::AcceptEntityFromCom(EntityComponent* entityCom, std::string comName)
	{
		if (entityCom != nullptr)
		{
			mEntityMap.clear();
			if(mEntityMap.find(comName) == mEntityMap.end())
			{			
				mEntityMap[comName].push_back(entityCom->getEntity());
				//MeshPathMap.emplace(comName, entity->mEntityPath);
			}
		}
	}

	void MaterialComponent::AcceptEntityFromCom(RoleComponent * roleCom, std::string comName)
	{
		if (roleCom != nullptr)
		{
			EntityMap tempEntityMap = roleCom->GetRole()->getEntityComponent()->getEntityMap();
			mEntityMap.clear();
			for (auto& it : tempEntityMap)
			{
				bool isExist = false;
				for (auto& entity : mEntityMap[comName])
				{
					if (it.second == entity)
						isExist = true;
				}
				if(!isExist)
					mEntityMap[comName].push_back(it.second);
			}
		}
	}

	std::string MaterialComponent::LoadFileFirst(MaterialComponentInfo* actorinfo)
	{
		std::string ResPath = "";
		if (m_isLoaded)
			return ResPath;
		std::string EntityComName = actorinfo->m_EntityComName;
		SetmEntityComName(EntityComName);
		std::string RoleComName = actorinfo->m_RoleComName;
		SetmRoleComName(RoleComName);
		for (EntityToSubEntityMap::iterator it = mEntityToSubEntityMap.begin(); it != mEntityToSubEntityMap.end(); it++)
		{
			for (auto& subEntity : it->second)
			{
				ReadMaterialOfSubEntity(subEntity);
				GetValueFromInfo(subEntity);
				SetCustomUniformParamsForSubEntity(subEntity);
			}
		}
		SetmCurrentEntity(actorinfo->m_CurrentEntityName);
		SetmCurrentSubEntity(actorinfo->m_CurrentSubEntityName);
		
		ReSetMessageVec();
		m_isLoaded = true;
		if (m_entityCom)
			ResPath = m_entityCom->mEntityPath.c_str();
		else if (m_RoleCom)
			ResPath = m_RoleCom->GetRolePath().c_str();
		return ResPath;
	}

	void MaterialComponent::SetShadeInfoVec()
	{
		/*TiXmlDocument doc;
		doc.Parse(buff);
		TiXmlElement * pRootElem = doc.RootElement();


		TiXmlElement* pElement = pRootElem->FirstChildElement("Shade");
		while (pElement)
		{
			ShadeInfo st;
			st.m_szShadeName = pElement->Attribute("name");
			st.m_szVsName = pElement->Attribute("vs_name");
			st.m_szPsName = pElement->Attribute("ps_name");
			m_vtShadeInfo.push_back(st);

			pElement = pElement->NextSiblingElement("Shade");
		}*/
	}

	void MaterialComponent::SetmCurrentEntity(std::string entityName)
	{
		if (mEntityMap.size() == 0)
			return;
		for (auto& entity : mEntityMap.begin()->second)
		{
			if (entity == nullptr)
				continue;
			if (strcmp(entityName.c_str(), entity->getName().c_str()) == 0)
			{
				m_entity = entity;
			}
		}
	}

	void MaterialComponent::SetmCurrentSubEntity(std::string subEntityName)
	{
		if (!m_entity)
			return;
		for (auto& subEntity : mEntityToSubEntityMap[m_entity])
		{
			if (strcmp(subEntityName.c_str(), subEntity->getMaterialName().c_str()) == 0)
			{
				m_subEntity = subEntity;
			}
		}
	}
	
	void MaterialComponent::SetmEntityComName(std::string EntityComName)
	{
		EntityComponent* entityCom = getActor()->GetComponent<EntityComponent>(EntityComName).get();
		AcceptEntityFromCom(entityCom, EntityComName);
		GetSubEntityOfEntity();
		m_entityCom = entityCom;
	}

	void MaterialComponent::SetmRoleComName(std::string RoleComName)
	{
		RoleComponent* roleCom = getActor()->GetComponent<RoleComponent>(RoleComName).get();
		AcceptEntityFromCom(roleCom, RoleComName);
		GetSubEntityOfEntity();
		m_RoleCom = roleCom;
	}

	void MaterialComponent::GetSubEntityOfEntity()
	{
		if (mEntityMap.empty())
			return;
		for (auto& entity : mEntityMap.begin()->second)
		{
			if (entity == nullptr)
				continue;
			Echo::uint32 size = entity->getNumSubEntities();

			for (Echo::uint32 i = 0; i < size; ++i)
			{
				Echo::SubEntity* subEntity = entity->getSubEntity(i);
				if (mEntityToSubEntityMap.find(entity) == mEntityToSubEntityMap.end())
					mEntityToSubEntityMap[entity].push_back(subEntity);
			}
		}	
	}


	//璇诲彇subentity鐨剆hader鍙傛暟锛屽弬鏁扮被鍨嬶紝鍊?
	void MaterialComponent::ReadMaterialOfSubEntity(SubEntity* subEntity)
	{
		if (!subEntity)
			return;
		Material* matPtr = subEntity->getMaterial();
		if (matPtr == nullptr)
		{
			return;
		}
		else if (!matPtr->check())
		{
			//std::string matErrPath = u8"\t鏉愯川鏂囦欢鐨勯《鐐圭浉鍏充俊鎭负绌烘垨鏄潃鑹插櫒瀹忎笌椤剁偣澹版槑涓嶄竴鑷?;
		}
		std::string shaderName = matPtr->m_matInfo.shadername;

		for (int i = 0; i < Echo::Shader_Type_Cnt; ++i)
		{
			if (m_curShaderParams[i])
			{
				if (m_curShaderParams[i]->size() != 0)
				{
					m_curShaderParams[i]->clear();
				}
			}
		}

		m_curShaderParams[Echo::Shader_Type_VS] = &matPtr->m_matInfo.vsShaderParams;
		m_curShaderParams[Echo::Shader_Type_PS] = &matPtr->m_matInfo.psShaderParams;

		Echo::CShaderManBin& pShaderBin = Echo::CShaderMan::getSingleton().GetShaderManBin();
		Echo::SShaderFXParams& fxParams = pShaderBin.mfGetFXParams(matPtr->GetShader());

		std::set<int> setCheck[Echo::Shader_Type_Cnt];

		Echo::SHADER_PARAMS tempShaderParams[Echo::Shader_Type_Cnt];
		for (int i = 0; i < Echo::Shader_Type_Cnt; ++i)
		{
			tempShaderParams[i].assign(m_curShaderParams[i]->begin(), m_curShaderParams[i]->end());
			m_curShaderParams[i]->clear();
		}

		//int index = 0;
		for (int i = 0; i < fxParams.m_FXParams.size(); ++i)
		{
			Echo::SFXParam& fxParam = fxParams.m_FXParams[i];

			mSubEntityToValueMap[subEntity].annotationsMesVec.push_back(fxParam.m_Annotations);
			mSubEntityToValueMap[subEntity].FXParamsVec.push_back(fxParam);

			if (fxParam.m_nFlags & PF_TWEAKABLE_MASK)
			{
				Echo::String strValues = fxParam.m_Values;

				float felement[4] = { 0.0,0.0,0.0,0.0 };
				sscanf(strValues.c_str(), "%f,%f,%f,%f", &felement[0], &felement[1], &felement[2], &felement[3]);

				short shaderIndex = 0;

				for (; shaderIndex < Echo::Shader_Type_Cnt; ++shaderIndex)
				{
					Echo::uint16 flag = Echo::uint16(1) << shaderIndex;
					if ((fxParam.m_nShaderFlag & flag) == 0)
						continue;
					short uniformIndex = fxParam.m_nUniformIdx[shaderIndex];
					if (uniformIndex != -1)
					{
						Echo::SHADER_PARAMS* pTempShaderParams = &tempShaderParams[shaderIndex];
						Echo::SHADER_PARAMS* pShaderParams = m_curShaderParams[shaderIndex];

						for (int j = 0; j < 3; ++j)
						{
							shaderParamVals shaderParamVal;
							shaderParamVal.shaderType = static_cast<Echo::EM_UNIFORM_TYPE>(shaderIndex);
							short paramIndex = fxParam.m_nParamIdx[shaderIndex][j];
							if (paramIndex != -1)
							{
								int temp = 10 * uniformIndex + paramIndex;

								if (setCheck[shaderIndex].find(temp) != setCheck[shaderIndex].end())
								{
									////MessageBox(NULL,L"shader鏂囦欢淇℃伅閿欒锛屽弬鏁伴噸澶?,L"閿欒淇℃伅",MB_OK);
									//QMessageBox::warning(this, QStringLiteral("閿欒淇℃伅"), QStringLiteral("shader鏂囦欢淇℃伅閿欒锛屽弬鏁伴噸澶?), QMessageBox::Ok);
								}
								else
								{
									setCheck[shaderIndex].insert(temp);
								}

								bool bSet = false;
								for (int i = 0; i < pTempShaderParams->size(); ++i)
								{
									Echo::st_UniformParam& stParam = pTempShaderParams->at(i);
									if (stParam.uniformIndex == uniformIndex && stParam.paramIndex == paramIndex)
									{
										felement[j] = stParam.paramVal;
										pShaderParams->push_back(stParam);

										shaderParamVal.uniformIndex = stParam.uniformIndex;
										shaderParamVal.paramIndex = stParam.paramIndex;
										shaderParamVal.paramVal = stParam.paramVal;

										mSubEntityToValueMap[subEntity].shaderParamValsVec.push_back(shaderParamVal);

										bSet = true;
										break;
									}
								}
								if (!bSet)
								{
									Echo::st_UniformParam stParam;
									stParam.uniformIndex = static_cast<Echo::uint8>(uniformIndex);
									stParam.paramIndex = static_cast<Echo::uint8>(paramIndex);
									stParam.paramVal = felement[j];
									pShaderParams->push_back(stParam);

									shaderParamVal.uniformIndex = stParam.uniformIndex;
									shaderParamVal.paramIndex = stParam.paramIndex;
									shaderParamVal.paramVal = stParam.paramVal;

									mSubEntityToValueMap[subEntity].shaderParamValsVec.push_back(shaderParamVal);
								}
							}
						}
					}
				}
			}
		}
		MaterialComponentInfo* actorinfo = static_cast<MaterialComponentInfo*>(m_pActComInfo);
		if (actorinfo->m_ValueMap.find(subEntity->getMaterialName()) == actorinfo->m_ValueMap.end())
		{
			for (auto& shaderParamVal : mSubEntityToValueMap[subEntity].shaderParamValsVec)
			{
				actorinfo->m_ValueMap[subEntity->getMaterialName()].push_back(shaderParamVal.paramVal);
			}
		}
	}

	Echo::String MaterialComponent::TranslateToChinese(Echo::String & name)
	{
		// TODO: 鍦ㄦ澶勬彃鍏?return 璇彞
		Echo::String ChineseName;
		auto It = ChineseMap.find(name);
		if (It != ChineseMap.end())
			ChineseName = It->second;
		if (ChineseName.empty())
			return name;
		return ChineseName;
	}

	void MaterialComponent::InitalizeChineseWords()
	{
		Echo::DataStreamPtr inStream(Echo::Root::instance()->GetFileDataStream("u8SceneBoxChineseWords.txt"));
		if (inStream.isNull())
		{
			LogManager::instance()->logMessage("SceneBoxChineseWords.txt Not Found In InitalizeChineseWords()", LML_CRITICAL);
			return;
		}
		while (!inStream->eof())
		{
			std::string aLine;
			aLine = inStream->getLine();
			size_t pos = aLine.find(" ");
			Echo::String keyName, srcName;
			if (pos != std::string::npos)
			{
				keyName = aLine.substr(0, pos);
				const size_t npos = keyName.find_last_not_of("\t");
				if (npos != Echo::String::npos)
					keyName = keyName.substr(0, npos + 1);
			}
			pos = aLine.find_last_of(" ");
			if (pos != std::string::npos)
			{
				srcName = aLine.substr(pos + 1, aLine.size());
				const size_t npos = srcName.find_last_of("\t");
				if (npos != Echo::String::npos)
					srcName = srcName.substr(0, npos);
			}
			ChineseMap.insert(make_pair(keyName, srcName));
		}
		inStream->close();
	}

	//閫氳繃entity缁勪欢鐨刴eshpath鍒ゆ柇鍔犺浇鐨勬ā鍨嬫槸鍚︽湁鏀瑰彉
	bool MaterialComponent::isResPathChange(std::string UIcomName)
	{
		/*	if (!entityCom1)
				return true;
			if (strcmp(entityCom1->mEntityPath.c_str(), MeshPathMap[comName].c_str()) != 0)
			{
				MeshPathMap.erase(comName);
				return true;
			}
			else*/
		if (m_RoleCom)
		{
			if (UIcomName.compare(m_RoleCom->GetRolePath().c_str()) == 0)
				return false;
			else
				return true;
		}
		else if (m_entityCom)
		{
			if (UIcomName.compare(m_entityCom->mEntityPath.c_str()) == 0)
				return false;
			else
				return true;
		}
		else
			return false;
	}

	void MaterialComponent::SetMaterialParam(MessageOfMaterialComponent* message)
	{
		if (message->iscolor)
		{
			Echo::shaderParamVals& shaderParamVal_z = mSubEntityToValueMap[m_subEntity].shaderParamValsVec[message->shaderValueIndex + 2];
			Echo::shaderParamVals& shaderParamVal_y = mSubEntityToValueMap[m_subEntity].shaderParamValsVec[message->shaderValueIndex + 1];
			Echo::shaderParamVals& shaderParamVal_x = mSubEntityToValueMap[m_subEntity].shaderParamValsVec[message->shaderValueIndex];
			m_subEntity->setCustomUniformParams(shaderParamVal_z.shaderType, shaderParamVal_z.uniformIndex, message->color);
			shaderParamVal_z.paramVal = message->color.z;
			shaderParamVal_y.paramVal = message->color.y;
			shaderParamVal_x.paramVal = message->color.x;
		}
		else
		{

			Echo::shaderParamVals& shaderParamVal = mSubEntityToValueMap[m_subEntity].shaderParamValsVec[message->shaderValueIndex];
			m_subEntity->setCustomUniformParams(shaderParamVal.shaderType, shaderParamVal.uniformIndex, shaderParamVal.paramIndex, message->val);
			shaderParamVal.paramVal = message->val;
		}
	}

	void MaterialComponent::ReSetMessageVec()
	{
		messageHandleVec.clear();
		if (!m_subEntity)
			return;
		int index = 0;
		for (int i = 0; i < mSubEntityToValueMap[m_subEntity].FXParamsVec.size(); ++i)
		{
			MessageOfMaterialComponent message;
			message.paramIndex = i;
			message.shaderValueIndex = index;
			if ((mSubEntityToValueMap[m_subEntity].FXParamsVec[i].m_nFlags & PF_TWEAKABLE_1) == 0)
			{
				message.iscolor = false;
				index++;
			}
			else
			{
				message.iscolor = true;
				index += 3;
			}
			messageHandleVec.push_back(message);
		}
	}

	void Echo::MaterialComponent::GetValueFromInfo(SubEntity* subEntity)
	{
		MaterialComponentInfo* actorinfo = static_cast<MaterialComponentInfo*>(m_pActComInfo);
		std::string subEntityName = subEntity->getMaterialName();
		auto valueVec = actorinfo->m_ValueMap.find(subEntityName);
		if (valueVec != actorinfo->m_ValueMap.end())
		{
			if (mSubEntityToValueMap.find(subEntity) != mSubEntityToValueMap.end())
			{
				int ParamIndex = 0;
				for (auto& shaderParamVals : mSubEntityToValueMap[subEntity].shaderParamValsVec)
				{
					shaderParamVals.paramVal = valueVec->second[ParamIndex];
					++ParamIndex;
				}
			}
		}
	}

	void MaterialComponent::SetCustomUniformParamsForSubEntity(SubEntity* subEntity)
	{
		if (mSubEntityToValueMap[subEntity].FXParamsVec.size() == 0)
			return;
		int index = 0;
		for (int i = 0; i < mSubEntityToValueMap[subEntity].FXParamsVec.size(); ++i)
		{
			auto shaderParamVal = mSubEntityToValueMap[subEntity].shaderParamValsVec[index];
			if ((mSubEntityToValueMap[subEntity].FXParamsVec[i].m_nFlags & PF_TWEAKABLE_1) == 0)
			{
				subEntity->setCustomUniformParams(shaderParamVal.shaderType, shaderParamVal.uniformIndex, shaderParamVal.paramIndex, shaderParamVal.paramVal);
				++index;
			}
			else
			{
				Vector4 color;
				color.x = shaderParamVal.paramVal;
				color.y = mSubEntityToValueMap[subEntity].shaderParamValsVec[index + 1].paramVal;
				color.z = mSubEntityToValueMap[subEntity].shaderParamValsVec[index + 2].paramVal;
				color.w = 0.0f;
				subEntity->setCustomUniformParams(shaderParamVal.shaderType, shaderParamVal.uniformIndex, color);
				index += 3;
			}
		}
	}

	MessageOfMaterialComponent MaterialComponent::FindMessageFromParamIndex(int paramIndex)
	{
		LoadFileFirst(static_cast<MaterialComponentInfo*>(m_pActComInfo));
		for (auto& messgae : messageHandleVec)
		{
			if (paramIndex == messgae.paramIndex)
				return messgae;
		}
		MessageOfMaterialComponent temp;
		temp.paramIndex = -1;
		return temp;
	}

	MaterialComponent * EchoMaterialScript::Script_GetMaterialComponent(ActorPtr  pActor, const std::string & name)
	{
		//ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		//ProtocolHandle comHandle = 0;
		//ActorComTypeDesc desc;
		//desc.actHandle = (Echo::ProtocolHandle)pActor;
		//desc.comType = COMPONENTTYPE(MaterialComponent);
		//desc.comName = name;
		//Echo::ProtocolHandle descHandle = (Echo::ProtocolHandle) &desc;
		//Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);
		//if (result != PR_OK)
		//	return nullptr;
		//if (comHandle == 0)
		//	return nullptr;

		//return (MaterialComponent*)comHandle;
		return nullptr;
	}
}; // namesapce Echo
