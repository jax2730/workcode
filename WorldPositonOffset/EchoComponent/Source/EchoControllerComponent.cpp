#include "EchoComponentStableHeaders.h"
#include "EchoControllerComponent.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoActor.h"
#include "EchoKeyBoard.h"
#include "EchoResourceFolderManager.h"
#include "EchoTimelineComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(ControllerComponent)

	EDITOR_REFLECT_BEGIN(ControllerComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_controllerType, "mControllerType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_filePath, "filePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	ControllerComponentInfo::ControllerComponentInfo(const Name& name, ActorCOMMoveableType moveable) : ActorComponentInfo(name, moveable)

	{

	}

	ControllerComponentInfo::~ControllerComponentInfo()
	{

	}
	void ControllerComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<ControllerComponentInfo>::get()->ExportJson(this, jsonNode, "");
		//if (!jsonNode)
		//	return;
		////controllerType(int)
		//cJSON_AddNumberToObject(jsonNode, "mControllerType", m_controllerType);
		////filePath
		//cJSON_AddStringToObject(jsonNode, "filePath", m_filePath.c_str());
	}
	void ControllerComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<ControllerComponentInfo>::get()->ImportJson(this, jsonNode, "");
		//if (!jsonNode)
		//	return;
		////controllerType
		//cJSON* Type = cJSON_GetObjectItem(jsonNode, "mControllerType");
		//if (Type)
		//	m_controllerType = (ControllerType)Type->valueint;
		////mControllerFilePath
		//cJSON* Path = cJSON_GetObjectItem(jsonNode, "filePath");
		//if (Path)
		//	m_filePath = Path->valuestring;
	}
	void ControllerComponentInfo::CopyTo(ActorComponentInfo * info)
	{
		ControllerComponentInfo* comInfo = dynamic_cast<ControllerComponentInfo*>(info);
		reflect::TypeResolver<ControllerComponentInfo>::get()->copyTo(this, comInfo);
		//comInfo->m_controllerType = m_controllerType;
		//comInfo->m_filePath = m_filePath;
	}

	void ControllerComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
		//if (comInfo == nullptr || comInfo->m_Type != m_Type)
		//	return;
		//ControllerComponentInfo* pInfo = static_cast<ControllerComponentInfo*> (comInfo);
		//OVERLAY_PROP(this, pInfo, controllerType);
		//OVERLAY_PROP(this, pInfo, filePath);
	}

	void ControllerComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			ControllerComponentInfo* oriInfo = static_cast<ControllerComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<ControllerComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}

	ControllerComponent::ControllerComponent(Echo::ActorPtr pActor, ActorCOMMoveableType isMoveable)
		:ProtocolActorComponent(pActor, isMoveable)
		, mControllerFilePath("")
		, mControllerType(ControllerType::entity)
		, mRun(false)
		
	{
	}

	ControllerComponent::~ControllerComponent()
	{

	}


	void ControllerComponent::init(ActorComponentInfo * info)
	{
		ProtocolActorComponent::init(info);
		m_pActComInfo = info;
		ControllerComponentInfo* ControllerComInfo = static_cast<ControllerComponentInfo*> (m_pActComInfo);
		if (ControllerComInfo == nullptr)
			return;
		SetControllerType((ControllerType)ControllerComInfo->GetcontrollerType());
		SetmControllerFilePath(ControllerComInfo->GetfilePath());                                                                                                        
		/*if (ControllerComInfo->TimelineFilePath != "")
		{
			GetTimeLineFile(TimeLineInfo->TimelineFilePath.data());
		}*/

	}

	void ControllerComponent::copyTo(const Echo::ActorComponentPtr& p_actor_component)
	{
	}

	void ControllerComponent::preRenderScene(EventID, ProtocolHandle handle)
	{
	}

	void ControllerComponent::postRenderScene(EventID, ProtocolHandle handle)
	{
	}

	void ControllerComponent::CreateControllerFile(std::string & filePath)
	{
		if (filePath.empty())
			return;
		cJSON* pRoot = cJSON_CreateObject();
		//ParametersVec
		cJSON* parameters = cJSON_CreateArray();
		cJSON_AddItemToObject(pRoot, "parameters", parameters);
		for (auto& parameter : mParametersDataVec)
		{
			cJSON* item = cJSON_CreateObject();
			cJSON_AddItemToArray(parameters, item);
			cJSON_AddNumberToObject(item, "parameterType", parameter.parameterType);
			cJSON_AddStringToObject(item, "parameterName", parameter.parameterName.c_str());
			cJSON_AddNumberToObject(item, "value_f", parameter.value_f);
			cJSON_AddNumberToObject(item, "value_i", parameter.value_i);
			cJSON_AddBoolToObject(item, "value_b", parameter.value_b);
			//key
			cJSON* key = cJSON_CreateArray();
			cJSON_AddItemToObject(item, "key", key);
			cJSON_AddNumberToObject(key, "key1", parameter.keyVec[0]);
			cJSON_AddNumberToObject(key, "key2", parameter.keyVec[1]);
		}
		//layer
		cJSON* layerVec = cJSON_CreateArray();
		cJSON_AddItemToObject(pRoot, "LayerVec", layerVec);
		for (auto& layer : mLayerDataVec)
		{
			cJSON* layerItem = cJSON_CreateObject();
			cJSON_AddItemToArray(layerVec, layerItem);
			//firstMachineData.name
			cJSON_AddStringToObject(layerItem, "layer_name", layer.firstMachineData.name.c_str());
			//firstMachineData.stateDataVec
			CreateStateDataItem(layerItem, layer.firstMachineData.stateDataVec);
		}
		char* out = cJSON_Print(pRoot);
		cJSON_Delete(pRoot);

		DataStreamPtr pDataStream(Echo::Root::instance()->CreateDataStream(filePath.c_str()));
		if (pDataStream.isNull())
		{
			free(out);
			return;
		}

		pDataStream->writeData(out, strlen(out));
		pDataStream->close();
		free(out);
		return;
	}

	void ControllerComponent::CreateStateDataItem(Echo::cJSON* item, std::vector<StateFileData*> stateFileData)
	{
		cJSON* stateVec = cJSON_CreateArray();
		cJSON_AddItemToObject(item, "StateVec", stateVec);
		for (auto& state : stateFileData)
		{
			cJSON* item = cJSON_CreateObject();
			cJSON_AddItemToArray(stateVec, item);
			//name
			cJSON_AddStringToObject(item, "name", state->name.c_str());
			//animationName_1
			cJSON_AddStringToObject(item, "animationName_1", state->animationName_1.c_str());
			//animationName_2
			cJSON_AddStringToObject(item, "animationName_2", state->animationName_2.c_str());
			//type
			cJSON_AddNumberToObject(item, "type", state->type);
			//sub_StateMachine
			if (state->type == State_Type_Sub_State)
			{
				CreateStateDataItem(item, state->sub_StateMachine.stateDataVec);
			}
			else if (state->type == State_Type_BlendTree)
			{
				BlendTreeFileData* blendTreeData = static_cast<BlendTreeFileData*>(state);
				cJSON_AddStringToObject(item, "roleCom", blendTreeData->roleCom.c_str());
				cJSON_AddBoolToObject(item, "windowOpen", blendTreeData->windowOpen);
				cJSON* blendtreeroot = cJSON_CreateArray();
				cJSON_AddItemToObject(item, "BlendTreeNode", blendtreeroot);
				cJSON* item = cJSON_CreateObject();
				cJSON_AddItemToArray(blendtreeroot, item);
				/*cJSON* blendtreeVec = cJSON_CreateObject();
				cJSON_AddItemToArray(blendtreeroot, blendtreeVec);*/
				CreateBlendTreeDataItem(item, blendTreeData->blendTree);
			}
			//pos
			cJSON* vec2Pos = cJSON_CreateArray();
			cJSON_AddItemToObject(item, "vec2Pos", vec2Pos);
			cJSON_AddNumberToObject(vec2Pos, "x", state->pos.x);
			cJSON_AddNumberToObject(vec2Pos, "y", state->pos.y);
			//isDefaultState
			cJSON_AddBoolToObject(item, "isDefaultState", state->isDefaultState);
			//connections
			cJSON* connections = cJSON_CreateArray();
			cJSON_AddItemToObject(item, "connections", connections);
			for (auto& connection : state->connections)
			{
				cJSON* item = cJSON_CreateObject();
				cJSON_AddItemToArray(connections, item);
				//InputState
				cJSON_AddStringToObject(item, "InputState", connection.InputState.c_str());
				//OutputState
				cJSON_AddStringToObject(item, "OutputState", connection.OutputState.c_str());
				//InputSlot
				cJSON_AddStringToObject(item, "InputSlot", connection.InputSlot.c_str());
				//OutputSlot
				cJSON_AddStringToObject(item, "OutputSlot", connection.OutputSlot.c_str());
				//ConditionVec
				cJSON* conditions = cJSON_CreateArray();
				cJSON_AddItemToObject(item, "conditions", conditions);
				for (auto& condition : connection.ConditionVec)
				{
					cJSON* item = cJSON_CreateObject();
					cJSON_AddItemToArray(conditions, item);
					//ParamName
					cJSON_AddStringToObject(item, "ParamName", condition.ParamName.c_str());
					//Term
					cJSON_AddNumberToObject(item, "Term", condition.Term);
					//f
					cJSON_AddNumberToObject(item, "f", condition.f);
					//i
					cJSON_AddNumberToObject(item, "i", condition.i);
					//b
					cJSON_AddBoolToObject(item, "b", condition.b);
				}
			}
		}
	}

	void ControllerComponent::CreateBlendTreeDataItem(Echo::cJSON * item, BlendTreeNodeFileData & blendTreeData)
	{
		
		/*cJSON* item = cJSON_CreateObject();
		cJSON_AddItemToArray(blendtreeVec, item);*/
		/*cJSON* item = cJSON_CreateObject();
		cJSON_AddItemToArray(item, blendtreeRoot);*/
		/*cJSON* blendtreeVec = cJSON_CreateArray();
		cJSON_AddItemToObject(blendtree, "BlendTreeNode", blendtreeVec);
		cJSON* item = cJSON_CreateObject();
		cJSON_AddItemToArray(blendtreeVec, item);*/

		/*cJSON* item = cJSON_CreateObject();
		cJSON_AddItemToArray(blendtreeroot, item);*/
		//name
		cJSON_AddStringToObject(item, "name", blendTreeData.name.c_str());
		//animationName_1
		cJSON_AddStringToObject(item, "animationName_1", blendTreeData.animationName_1.c_str());
		//animationName_2
		cJSON_AddStringToObject(item, "animationName_2", blendTreeData.animationName_2.c_str());
		//type
		cJSON_AddNumberToObject(item, "type", blendTreeData.type);
		//pos
		cJSON* vec2Pos = cJSON_CreateArray();
		cJSON_AddItemToObject(item, "vec2Pos", vec2Pos);
		cJSON_AddNumberToObject(vec2Pos, "x", blendTreeData.pos.x);
		cJSON_AddNumberToObject(vec2Pos, "y", blendTreeData.pos.y);
		//isDefaultState
		cJSON_AddBoolToObject(item, "isDefaultState", blendTreeData.isDefaultState);
		//BlendTreeData
		cJSON_AddStringToObject(item, "pos_x", blendTreeData.pos_x.c_str());
		cJSON_AddStringToObject(item, "pos_y", blendTreeData.pos_y.c_str());
		cJSON_AddNumberToObject(item, "weight", blendTreeData.weight);
		cJSON* childblendtree_pos = cJSON_CreateArray();
		cJSON_AddItemToObject(item, "childblendtree_pos", childblendtree_pos);
		cJSON_AddNumberToObject(childblendtree_pos, "childblendtree_pos_x", blendTreeData.childblendtree_pos.x);
		cJSON_AddNumberToObject(childblendtree_pos, "childblendtree_pos_y", blendTreeData.childblendtree_pos.y);
		cJSON* behaviorVec = cJSON_CreateArray();
		cJSON_AddItemToObject(item, "Behavior", behaviorVec);
		for (auto& behavior : blendTreeData.BehaviorList)
		{
			cJSON* behaviorJson = cJSON_CreateObject();
			cJSON_AddItemToArray(behaviorVec, behaviorJson);
			
			cJSON_AddStringToObject(behaviorJson, "name", behavior.name.c_str());
			cJSON_AddStringToObject(behaviorJson, "animationName_1", behavior.animationName_1.c_str());
			cJSON_AddStringToObject(behaviorJson, "animationName_2", behavior.animationName_2.c_str());
			cJSON_AddNumberToObject(behaviorJson, "animationIndex1", behavior.animationIndex1);
			cJSON_AddNumberToObject(behaviorJson, "animationIndex2", behavior.animationIndex2);
			cJSON_AddNumberToObject(behaviorJson, "type", behavior.type);
			cJSON* vec2Pos = cJSON_CreateArray();
			cJSON_AddItemToObject(behaviorJson, "vec2Pos", vec2Pos);
			cJSON_AddNumberToObject(vec2Pos, "x", behavior.pos.x);
			cJSON_AddNumberToObject(vec2Pos, "y", behavior.pos.y);
			cJSON_AddBoolToObject(behaviorJson, "isDefaultState", behavior.isDefaultState);

			cJSON_AddNumberToObject(behaviorJson, "pos_x", behavior.pos_x);
			cJSON_AddNumberToObject(behaviorJson, "pos_y", behavior.pos_y);
			cJSON_AddNumberToObject(behaviorJson, "weight", behavior.weight);
			//cJSON* connections = cJSON_CreateArray();
			//cJSON_AddItemToObject(behaviorJson, "connections", connections);
			//for (auto& connection : behavior.connections)
			//{
			//	cJSON* item = cJSON_CreateObject();
			//	cJSON_AddItemToArray(connections, item);
			//	//InputState
			//	cJSON_AddStringToObject(item, "InputState", connection.InputState.c_str());
			//	//OutputState
			//	cJSON_AddStringToObject(item, "OutputState", connection.OutputState.c_str());
			//	//InputSlot
			//	cJSON_AddStringToObject(item, "InputSlot", connection.InputSlot.c_str());
			//	//OutputSlot
			//	cJSON_AddStringToObject(item, "OutputSlot", connection.OutputSlot.c_str());
			//	//ConditionVec
			//	cJSON* conditions = cJSON_CreateArray();
			//	cJSON_AddItemToObject(item, "conditions", conditions);
			//	for (auto& condition : connection.ConditionVec)
			//	{
			//		cJSON* item = cJSON_CreateObject();
			//		cJSON_AddItemToArray(conditions, item);
			//		//ParamName
			//		cJSON_AddStringToObject(item, "ParamName", condition.ParamName.c_str());
			//		//Term
			//		cJSON_AddNumberToObject(item, "Term", condition.Term);
			//		//f
			//		cJSON_AddNumberToObject(item, "f", condition.f);
			//		//i
			//		cJSON_AddNumberToObject(item, "i", condition.i);
			//		//b
			//		cJSON_AddBoolToObject(item, "b", condition.b);
			//	}
			//}
		}
		////connections
		//cJSON* connections = cJSON_CreateArray();
		//cJSON_AddItemToObject(item, "connections", connections);
		//for (auto& connection : blendTreeData.connections)
		//{
		//	cJSON* item = cJSON_CreateObject();
		//	cJSON_AddItemToArray(connections, item);
		//	//InputState
		//	cJSON_AddStringToObject(item, "InputState", connection.InputState.c_str());
		//	//OutputState
		//	cJSON_AddStringToObject(item, "OutputState", connection.OutputState.c_str());
		//	//InputSlot
		//	cJSON_AddStringToObject(item, "InputSlot", connection.InputSlot.c_str());
		//	//OutputSlot
		//	cJSON_AddStringToObject(item, "OutputSlot", connection.OutputSlot.c_str());
		//	//ConditionVec
		//	cJSON* conditions = cJSON_CreateArray();
		//	cJSON_AddItemToObject(item, "conditions", conditions);
		//	for (auto& condition : connection.ConditionVec)
		//	{
		//		cJSON* item = cJSON_CreateObject();
		//		cJSON_AddItemToArray(conditions, item);
		//		//ParamName
		//		cJSON_AddStringToObject(item, "ParamName", condition.ParamName.c_str());
		//		//Term
		//		cJSON_AddNumberToObject(item, "Term", condition.Term);
		//		//f
		//		cJSON_AddNumberToObject(item, "f", condition.f);
		//		//i
		//		cJSON_AddNumberToObject(item, "i", condition.i);
		//		//b
		//		cJSON_AddBoolToObject(item, "b", condition.b);
		//	}
		//}
		cJSON* childBlendtreeVec = cJSON_CreateArray();
		cJSON_AddItemToObject(item, "ChildBlendTreeNode", childBlendtreeVec);
		for (auto& blendtree : blendTreeData.BlendtreeList)
		{
			cJSON* item = cJSON_CreateObject();
			cJSON_AddItemToArray(childBlendtreeVec, item);
			CreateBlendTreeDataItem(item, blendtree);
		}
	}

	void ControllerComponent::ReadControllerFile(std::string & filePath)
	{
		mParametersDataVec.clear();
		mLayerDataVec.clear();
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(filePath.c_str()));
		if (pDataStream.isNull())
		{
			return;
		}
		size_t nSize = pDataStream->size();
		char *pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->read(pData, nSize);
		cJSON* pRoot = cJSON_Parse(pData);

		if (pRoot == nullptr)
			return;
		//ParametersVec
		cJSON* ParameterVec = cJSON_GetObjectItem(pRoot, "parameters");
		if (ParameterVec)
		{
			int parameterSize = cJSON_GetArraySize(ParameterVec);
			for (int i = 0; i < parameterSize; ++i)
			{
				cJSON* item = cJSON_GetArrayItem(ParameterVec, i);
				ParameterFileData parameterFileData;
				cJSON* parameterType = cJSON_GetObjectItem(item, "parameterType");
				cJSON* parameterName = cJSON_GetObjectItem(item, "parameterName");
				cJSON* value_f = cJSON_GetObjectItem(item, "value_f");
				cJSON* value_i = cJSON_GetObjectItem(item, "value_i");
				cJSON* value_b = cJSON_GetObjectItem(item, "value_b");
				cJSON* key = cJSON_GetObjectItem(item, "key");
				if (key)
				{
					cJSON* key1 = cJSON_GetArrayItem(key, 0);
					if(key1)
						parameterFileData.keyVec[0] = key1->valueint;
					cJSON* key2 = cJSON_GetArrayItem(key, 1);
					if (key2)
						parameterFileData.keyVec[1] = key2->valueint;
				}
				std::string ParameterName = parameterName->valuestring;
				int index = 0;
				while (!IsChangeParameterName(ParameterName))
				{
					ParameterName = parameterName->valuestring + std::to_string(index);
					++index;
				}
				parameterFileData.parameterName = parameterName->valuestring;
				parameterFileData.parameterType = (Echo::Type)parameterType->valueint;
				parameterFileData.value_f = (float)value_f->valuedouble;
				parameterFileData.value_i = value_i->valueint;
				if (value_b->valueint == cJSON_False)
					parameterFileData.value_b = false;
				else
					parameterFileData.value_b = true;
				mParametersDataVec.push_back(parameterFileData);
			}
		}
		//layerVec
		cJSON* layerVec = cJSON_GetObjectItem(pRoot, "LayerVec");	
		if (layerVec)
		{
			int layerSize = cJSON_GetArraySize(layerVec);
			for (int i = 0; i < layerSize; ++i)
			{
				cJSON* layerItem = cJSON_GetArrayItem(layerVec, i);
				LayerFileData layerInfoData;
				//layer_name
				cJSON* layer_name = cJSON_GetObjectItem(layerItem, "layer_name");
				//stateFileData
				cJSON* stateVec = cJSON_GetObjectItem(layerItem, "StateVec");
				layerInfoData.firstMachineData.name = layer_name->valuestring;

				for (auto& layer : mLayerDataVec)
				{
					int index = 0;
					while (layerInfoData.firstMachineData.name == layer.firstMachineData.name)
					{
						layerInfoData.firstMachineData.name = layer_name->valuestring + std::to_string(index);
						++index;
					}
				}
				
				ReadStateDataItem(stateVec, layerInfoData.firstMachineData.stateDataVec);
				mLayerDataVec.push_back(layerInfoData);
			}
		}
	}

	void ControllerComponent::ReadStateDataItem(cJSON * stateVec, std::vector<StateFileData*>& stateDataVec)
	{
		if (stateVec)
		{
			int stateSize = cJSON_GetArraySize(stateVec);
			for (int i = 0; i < stateSize; ++i)
			{
				BlendTreeFileData* stateFileData = new BlendTreeFileData();
				cJSON* item = cJSON_GetArrayItem(stateVec, i);

				cJSON* name = cJSON_GetObjectItem(item, "name");
				stateFileData->name = name->valuestring;
				cJSON* animationName_1 = cJSON_GetObjectItem(item, "animationName_1");
				stateFileData->animationName_1 = animationName_1->valuestring;
				cJSON* animationName_2 = cJSON_GetObjectItem(item, "animationName_2");
				stateFileData->animationName_2 = animationName_2->valuestring;
				cJSON* type = cJSON_GetObjectItem(item, "type");
				stateFileData->type = (State_Type)type->valueint;
				if (stateFileData->type == State_Type_Sub_State)
				{
					cJSON* stateVec = cJSON_GetObjectItem(item, "StateVec");
					ReadStateDataItem(stateVec, stateFileData->sub_StateMachine.stateDataVec);
				}
				else if (stateFileData->type == State_Type_BlendTree)
				{
					//BlendTreeFileData* blendtreeFileData = static_cast<BlendTreeFileData*>(stateFileData);
					cJSON* roleCom = cJSON_GetObjectItem(item, "roleCom");
					stateFileData->roleCom = roleCom->valuestring;
					cJSON* windowOpen = cJSON_GetObjectItem(item, "windowOpen");
					if (windowOpen->valueint == cJSON_False)
						stateFileData->windowOpen = false;
					else
						stateFileData->windowOpen = true;
					cJSON* blendtreeVec = cJSON_GetObjectItem(item, "BlendTreeNode");
					if (blendtreeVec)
					{
						cJSON* blendtree = cJSON_GetArrayItem(blendtreeVec, 0);
						if(blendtree)
							ReadBlendTreeDataItem(blendtree, stateFileData->blendTree);
					}
				}
				cJSON* vec2Pos = cJSON_GetObjectItem(item, "vec2Pos");
				cJSON* x = cJSON_GetArrayItem(vec2Pos, 0);
				stateFileData->pos.x = (float)x->valuedouble;
				cJSON* y = cJSON_GetArrayItem(vec2Pos, 1);
				stateFileData->pos.y = (float)y->valuedouble;
				cJSON* isDefaultState = cJSON_GetObjectItem(item, "isDefaultState");
				if (isDefaultState->valueint == cJSON_False)
					stateFileData->isDefaultState = false;
				else
					stateFileData->isDefaultState = true;
				//connections
				cJSON* connections = cJSON_GetObjectItem(item, "connections");
				int connectionSize = cJSON_GetArraySize(connections);
				if (connections)
				{
					for (int j = 0; j < connectionSize; ++j)
					{
						cJSON* item = cJSON_GetArrayItem(connections, j);
						ConnectionFileData connectionFileData;
						cJSON* InputState = cJSON_GetObjectItem(item, "InputState");
						connectionFileData.InputState = InputState->valuestring;
						cJSON* OutputState = cJSON_GetObjectItem(item, "OutputState");
						connectionFileData.OutputState = OutputState->valuestring;
						cJSON* InputSlot = cJSON_GetObjectItem(item, "InputSlot");
						connectionFileData.InputSlot = InputSlot->valuestring;
						cJSON* OutputSlot = cJSON_GetObjectItem(item, "OutputSlot");
						connectionFileData.OutputSlot = OutputSlot->valuestring;
						//ConditionVec
						cJSON* conditions = cJSON_GetObjectItem(item, "conditions");
						int conditionSize = cJSON_GetArraySize(conditions);
						if (conditions)
						{
							for (int k = 0; k < conditionSize; ++k)
							{
								cJSON* item = cJSON_GetArrayItem(conditions, k);
								ConditionFileData conditionFileData;
								cJSON* ParamName = cJSON_GetObjectItem(item, "ParamName");
								conditionFileData.ParamName = ParamName->valuestring;
								cJSON* Term = cJSON_GetObjectItem(item, "Term");
								conditionFileData.Term = (Echo::Terms)Term->valueint;
								cJSON* f = cJSON_GetObjectItem(item, "f");
								conditionFileData.f = (float)f->valuedouble;
								cJSON* i = cJSON_GetObjectItem(item, "i");
								conditionFileData.i = i->valueint;
								cJSON* b = cJSON_GetObjectItem(item, "b");
								if (b->valueint == cJSON_False)
									conditionFileData.b = false;
								else
									conditionFileData.b = true;
								connectionFileData.ConditionVec.push_back(conditionFileData);
							}
						}
						stateFileData->connections.push_back(connectionFileData);
					}
				}
				stateDataVec.push_back(stateFileData);
			}
		}
	}

	void ControllerComponent::ReadBlendTreeDataItem(cJSON * blendtreeVec, BlendTreeNodeFileData & blendTreeData)
	{
		//cJSON* blendtree = cJSON_GetArrayItem(blendtreeVec, i);

		cJSON* name = cJSON_GetObjectItem(blendtreeVec, "name");
		blendTreeData.name = name->valuestring;
		cJSON* animationName_1 = cJSON_GetObjectItem(blendtreeVec, "animationName_1");
		blendTreeData.animationName_1 = animationName_1->valuestring;
		cJSON* animationName_2 = cJSON_GetObjectItem(blendtreeVec, "animationName_2");
		blendTreeData.animationName_2 = animationName_2->valuestring;
		cJSON* type = cJSON_GetObjectItem(blendtreeVec, "type");
		blendTreeData.type = (State_Type)type->valueint;
		cJSON* vec2Pos = cJSON_GetObjectItem(blendtreeVec, "vec2Pos");
		cJSON* x = cJSON_GetArrayItem(vec2Pos, 0);
		blendTreeData.pos.x = (float)x->valuedouble;
		cJSON* y = cJSON_GetArrayItem(vec2Pos, 1);
		blendTreeData.pos.y = (float)y->valuedouble;
		cJSON* isDefaultState = cJSON_GetObjectItem(blendtreeVec, "isDefaultState");
		if (isDefaultState->valueint == cJSON_False)
			blendTreeData.isDefaultState = false;
		else
			blendTreeData.isDefaultState = true;
		cJSON* pos_x = cJSON_GetObjectItem(blendtreeVec, "pos_x");
		blendTreeData.pos_x = pos_x->valuestring;
		cJSON* pos_y = cJSON_GetObjectItem(blendtreeVec, "pos_y");
		blendTreeData.pos_y = pos_y->valuestring;
		cJSON* weight = cJSON_GetObjectItem(blendtreeVec, "weight");
		blendTreeData.weight = (float)weight->valuedouble;
		cJSON* childblendtree_pos = cJSON_GetObjectItem(blendtreeVec, "childblendtree_pos");
		cJSON* childblendtree_pos_x = cJSON_GetArrayItem(childblendtree_pos, 0);
		blendTreeData.childblendtree_pos.x = (float)childblendtree_pos_x->valuedouble;
		cJSON* childblendtree_pos_y = cJSON_GetArrayItem(childblendtree_pos, 1);
		blendTreeData.childblendtree_pos.y = (float)childblendtree_pos_y->valuedouble;
		cJSON* behaviorVec = cJSON_GetObjectItem(blendtreeVec, "Behavior");
		if (behaviorVec)
		{
			int behaviorSize = cJSON_GetArraySize(behaviorVec);
			for (int m = 0; m < behaviorSize; ++m)
			{
				cJSON* behavior = cJSON_GetArrayItem(behaviorVec, m);
				BehaviorFildData behaviorFileData;
				cJSON* name = cJSON_GetObjectItem(behavior, "name");
				behaviorFileData.name = name->valuestring;
				cJSON* animationName_1 = cJSON_GetObjectItem(behavior, "animationName_1");
				behaviorFileData.animationName_1 = animationName_1->valuestring;
				cJSON* animationName_2 = cJSON_GetObjectItem(behavior, "animationName_2");
				behaviorFileData.animationName_2 = animationName_2->valuestring;
				cJSON* animationIndex1 = cJSON_GetObjectItem(behavior, "animationIndex1");
				behaviorFileData.animationIndex1 = animationIndex1->valueint;
				cJSON* animationIndex2 = cJSON_GetObjectItem(behavior, "animationIndex2");
				behaviorFileData.animationIndex2 = animationIndex2->valueint;
				cJSON* type = cJSON_GetObjectItem(behavior, "type");
				behaviorFileData.type = (State_Type)type->valueint;
				cJSON* vec2Pos = cJSON_GetObjectItem(behavior, "vec2Pos");
				cJSON* x = cJSON_GetArrayItem(vec2Pos, 0);
				behaviorFileData.pos.x = (float)x->valuedouble;
				cJSON* y = cJSON_GetArrayItem(vec2Pos, 1);
				behaviorFileData.pos.y = (float)y->valuedouble;
				cJSON* isDefaultState = cJSON_GetObjectItem(behavior, "isDefaultState");
				if (isDefaultState->valueint == cJSON_False)
					behaviorFileData.isDefaultState = false;
				else
					behaviorFileData.isDefaultState = true;
				cJSON* pos_x = cJSON_GetObjectItem(behavior, "pos_x");
				behaviorFileData.pos_x = (float)pos_x->valuedouble;
				cJSON* pos_y = cJSON_GetObjectItem(behavior, "pos_y");
				behaviorFileData.pos_y = (float)pos_y->valuedouble;
				cJSON* weight = cJSON_GetObjectItem(behavior, "weight");
				behaviorFileData.weight = (float)weight->valuedouble;
				////connections
				//cJSON* connections = cJSON_GetObjectItem(behavior, "connections");
				//int connectionSize = cJSON_GetArraySize(connections);
				//if (connections)
				//{
				//	for (int n = 0; n < connectionSize; ++n)
				//	{
				//		cJSON* item = cJSON_GetArrayItem(connections, n);
				//		ConnectionFileData connectionFileData;
				//		cJSON* InputState = cJSON_GetObjectItem(item, "InputState");
				//		connectionFileData.InputState = InputState->valuestring;
				//		cJSON* OutputState = cJSON_GetObjectItem(item, "OutputState");
				//		connectionFileData.OutputState = OutputState->valuestring;
				//		cJSON* InputSlot = cJSON_GetObjectItem(item, "InputSlot");
				//		connectionFileData.InputSlot = InputSlot->valuestring;
				//		cJSON* OutputSlot = cJSON_GetObjectItem(item, "OutputSlot");
				//		connectionFileData.OutputSlot = OutputSlot->valuestring;
				//		//ConditionVec
				//		cJSON* conditions = cJSON_GetObjectItem(item, "conditions");
				//		int conditionSize = cJSON_GetArraySize(conditions);
				//		if (conditions)
				//		{
				//			for (int k = 0; k < conditionSize; ++k)
				//			{
				//				cJSON* item = cJSON_GetArrayItem(conditions, k);
				//				ConditionFileData conditionFileData;
				//				cJSON* ParamName = cJSON_GetObjectItem(item, "ParamName");
				//				conditionFileData.ParamName = ParamName->valuestring;
				//				cJSON* Term = cJSON_GetObjectItem(item, "Term");
				//				conditionFileData.Term = (Echo::Terms)Term->valueint;
				//				cJSON* f = cJSON_GetObjectItem(item, "f");
				//				conditionFileData.f = (float)f->valuedouble;
				//				cJSON* i = cJSON_GetObjectItem(item, "i");
				//				conditionFileData.i = i->valueint;
				//				cJSON* b = cJSON_GetObjectItem(item, "b");
				//				if (b->valueint == cJSON_False)
				//					conditionFileData.b = false;
				//				else
				//					conditionFileData.b = true;
				//				connectionFileData.ConditionVec.push_back(conditionFileData);
				//			}
				//		}
				//		behaviorFileData.connections.push_back(connectionFileData);
				//	}
				//}
				blendTreeData.BehaviorList.push_back(behaviorFileData);
			}
		}

		////connections
		//cJSON* connections = cJSON_GetObjectItem(blendtreeVec, "connections");
		//int connectionSize = cJSON_GetArraySize(connections);
		//if (connections)
		//{
		//	for (int j = 0; j < connectionSize; ++j)
		//	{
		//		cJSON* item = cJSON_GetArrayItem(connections, j);
		//		ConnectionFileData connectionFileData;
		//		cJSON* InputState = cJSON_GetObjectItem(item, "InputState");
		//		connectionFileData.InputState = InputState->valuestring;
		//		cJSON* OutputState = cJSON_GetObjectItem(item, "OutputState");
		//		connectionFileData.OutputState = OutputState->valuestring;
		//		cJSON* InputSlot = cJSON_GetObjectItem(item, "InputSlot");
		//		connectionFileData.InputSlot = InputSlot->valuestring;
		//		cJSON* OutputSlot = cJSON_GetObjectItem(item, "OutputSlot");
		//		connectionFileData.OutputSlot = OutputSlot->valuestring;
		//		//ConditionVec
		//		cJSON* conditions = cJSON_GetObjectItem(item, "conditions");
		//		int conditionSize = cJSON_GetArraySize(conditions);
		//		if (conditions)
		//		{
		//			for (int s = 0; s < conditionSize; ++s)
		//			{
		//				cJSON* item = cJSON_GetArrayItem(conditions, s);
		//				ConditionFileData conditionFileData;
		//				cJSON* ParamName = cJSON_GetObjectItem(item, "ParamName");
		//				conditionFileData.ParamName = ParamName->valuestring;
		//				cJSON* Term = cJSON_GetObjectItem(item, "Term");
		//				conditionFileData.Term = (Echo::Terms)Term->valueint;
		//				cJSON* f = cJSON_GetObjectItem(item, "f");
		//				conditionFileData.f = (float)f->valuedouble;
		//				cJSON* i = cJSON_GetObjectItem(item, "i");
		//				conditionFileData.i = i->valueint;
		//				cJSON* b = cJSON_GetObjectItem(item, "b");
		//				if (b->valueint == cJSON_False)
		//					conditionFileData.b = false;
		//				else
		//					conditionFileData.b = true;
		//				connectionFileData.ConditionVec.push_back(conditionFileData);
		//			}
		//		}
		//		blendTreeData.connections.push_back(connectionFileData);
		//	}
		//}
		cJSON* childblendtreeVec = cJSON_GetObjectItem(blendtreeVec, "ChildBlendTreeNode");
		if (childblendtreeVec)
		{
			int childBlendtreeSize = cJSON_GetArraySize(childblendtreeVec);
			for (int i = 0; i < childBlendtreeSize; ++i)
			{
				cJSON* childBlendtree = cJSON_GetArrayItem(childblendtreeVec, i);
				BlendTreeNodeFileData childBlendtreeFildData;
				ReadBlendTreeDataItem(childBlendtree, childBlendtreeFildData);
				blendTreeData.BlendtreeList.push_back(childBlendtreeFildData);
			}
		}
	}

	void ControllerComponent::SetmControllerFilePath(std::string filePath)
	{
		mControllerFilePath = filePath;
		if(!mControllerFilePath.empty())
			ReadControllerFile(filePath);
	}

	void ControllerComponent::Transiting()
	{
		if (!mRun)
			return;
		if (!mLayer)
			mLayer = *(mLayerVec.begin());
		/*if(!mStateMachine)
			mStateMachine = mLayer->mFirstStateMachine;
		if(!mState)
			mState = *(mStateMachine->mStatesVec.begin());*/
		mLayer->mFirstStateMachine->PlayInMachine();
	}

	void ControllerComponent::PlayInStateMachine(StateMachine* stateMachine)
	{
		/*mState = *(stateMachine->mStatesVec.begin());
		for (auto connection : mState->Connections)
		{
			if (connection->InputState == mState)
				continue;
			connection->SetMet();
			if (connection->isMet)
			{
				mState = static_cast<State*>(connection->InputState);
				if (mState->Type == State_Type_Exit)
				{
					mRun = false;
					mState = nullptr;
					return;
				}

				else
				{
					mState->Play();
					break;
				}
			}
		}*/
	}

	void ControllerComponent::AddLayer(StateMachine* firstStateMachine)
	{
		mLayerVec.push_back(new Layer(firstStateMachine));
	}

	void ControllerComponent::AddParameterVec(std::string& ParameterName, ParamtersAttribute* Attribute)
	{
		SetParameterName(ParameterName);
		Parameter* parameter = new Parameter(ParameterName, Attribute);
		mParametersVec.push_back(parameter);
	}

	void ControllerComponent::ClearLayers()
	{
		if (mLayerVec.empty())
			return;
		for (auto& layer : mLayerVec)
		{
			if (!layer)
				continue;
			SAFE_DELETE(layer);
		}
		mLayerVec.clear();
		mLayerVec.shrink_to_fit();
	}

	void ControllerComponent::ClearParmeterVec()
	{
		if (mParametersVec.empty())
			return;
		for (auto& parameter : mParametersVec)
		{
			if (!parameter)
				continue;
			SAFE_DELETE(parameter);
		}
		mParametersVec.clear();
		mParametersVec.shrink_to_fit();
	}

	void ControllerComponent::SetParameterName(std::string & ParameterName)
	{
		std::string tempName = ParameterName;
		int index = 0;
		for (auto paramter : mParametersVec)
		{
			if (!paramter)
				continue;
			while (strcmp(ParameterName.c_str(), paramter->Name.c_str()) == 0)
			//if (strcmp(ParameterName.c_str(), paramter->Name.c_str()) == 0)
			{
				ParameterName = tempName + std::to_string(index);
				++index;
			}
		}
	}

	void ControllerComponent::EnterMachinePath(StateMachine* orderMachine)
	{
		mStateMachinePath += "->";
		mStateMachinePath += orderMachine->mName.c_str();
		mStateMachine = orderMachine;
	}

	void ControllerComponent::ReturnMachinePath()
	{
		mStateMachinePath = mStateMachinePath.substr(0, mStateMachinePath.find_last_of("-"));
		mStateMachine = mStateMachine->mUpStateMachine;
	}

	bool ControllerComponent::IsChangeParameterName(std::string name)
	{
		if (name.empty())
			return false;
		for (auto& iter : mParametersVec)
		{
			if (iter->Name == name)
				return false;
		}
		return true;
	}

	//void ControllerComponent::ControllerKeyDown(int key)
	//{
	//	if (!mIsPlay)
	//		return;
	//	EchoKeyBoard::instance()->GetKeyBoard(key] = true;
	//	//for (auto& iter : EchoKeyBoard::instance()->KeyboardValueMap)
	//	//{
	//	//	if (iter.first == key)
	//	//		iter.second = true;
	//	//}
	//}

	//void ControllerComponent::ControllerKeyUp(int key)
	//{
	//	if (!mIsPlay)
	//		return;
	//	EchoKeyBoard::instance()->GetKeyBoard(key] = false;
	//	//for (auto iter : EchoKeyBoard::instance()->KeyboardValueMap)
	//	//{
	//	//	if (iter.first == key)
	//	//		iter.second = false;
	//	//}
	//}

	bool ControllerComponent::SetParameterIntValue(int vecIndex, int value)
	{
		if (mParametersVec.empty() || vecIndex >= (int)mParametersVec.size())
			return false;
		if (mParametersVec[vecIndex]->Attribute->ParamtersAttribute::type != Type::ParameterINT)
			return false;
		*(int*)(mParametersVec[vecIndex]->Attribute->value) = value;
		return true;
	}

	bool ControllerComponent::SetParameterFloatValue(int vecIndex, float value)
	{
		if (mParametersVec.empty() || vecIndex >= (int)mParametersVec.size())
			return false;
		if (mParametersVec[vecIndex]->Attribute->ParamtersAttribute::type != Type::ParameterFLOAT)
			return false;
		*(float*)(mParametersVec[vecIndex]->Attribute->value) = value;
		return true;
	}

	float ControllerComponent::GetParameterFloatValue(int vecIndex)
	{
		if (mParametersVec.empty())
			return 0.0f;
		if (vecIndex >= (int)mParametersVec.size())
			return 0.0f;
		if (!mParametersVec[vecIndex])
			return 0.0f;
		if (mParametersVec[vecIndex]->Attribute->ParamtersAttribute::type != Type::ParameterFLOAT)
			return 0.0f;
		return *(float*)(mParametersVec[vecIndex]->Attribute->value);
	}

	float ControllerComponent::GetParameterFloatValue_Name(std::string name)
	{
		if (mParametersVec.empty())
			return 0.0f;
		for (auto& iter : mParametersVec)
		{
			if (!iter)
				continue;
			if (iter->Name == name)
			{
				if (iter->Attribute->type != ParameterFLOAT)
					return 0.0f;
				return *(float*)(iter->Attribute->value);
			}
		}
		return 0.0f;
	}

	bool ControllerComponent::SetParameterFloatValue_Name(std::string name, float value)
	{
		if (name.empty())
			return false;
		for (auto& iter : mParametersVec)
		{
			if (!iter)
				continue;
			if (iter->Name == name)
			{
				if (iter->Attribute->type != ParameterFLOAT)
					return false;
				*(float*)(iter->Attribute->value) = value;
				return true;
			}
		}
		return false;
	}

	bool ControllerComponent::SetParameterBoolValue_Key(int key)
	{
		if (key == 0)
			return false;

		bool change = false;
		for (auto& iter : mParametersVec)
		{
			if (!iter)
				continue;
			if (iter->Attribute->ParamtersKeyboard(key) && iter->Attribute->type == ParameterBOOL)
			{			
				if (*(bool*)(iter->Attribute->value) != EchoKeyBoard::instance()->GetKeyBoard(key))
				{
					*(bool*)(iter->Attribute->value) = EchoKeyBoard::instance()->GetKeyBoard(key);
					change = true;
				}	
			}
		}
		return change;
	}

	bool ControllerComponent::SetParameterFloatValue_Key_Limit(int key, float speed, float limit)
	{
		if (key == 0 || speed < 0.0f)
			return false;
		bool change = false;
		for (auto& iter : mParametersVec)
		{
			if (!iter)
				continue;
			if (iter->Attribute->ParamtersKeyboard(key) && iter->Attribute->type == ParameterFLOAT)
			{
				if (*(float*)(iter->Attribute->value) == limit)
					continue;
				else if (*(float*)(iter->Attribute->value) < limit)
				{
					*(float*)(iter->Attribute->value) += speed;
					if ((*(float*)(iter->Attribute->value) > limit))
						*(float*)(iter->Attribute->value) = limit;
				}
				else if ((*(float*)(iter->Attribute->value) > limit))
				{
					*(float*)(iter->Attribute->value) -= speed;
					if (*(float*)(iter->Attribute->value) < limit)
						*(float*)(iter->Attribute->value) = limit;
				}
				change = true;
			}
		}
		return change;
	}

	bool ControllerComponent::SetParameterFloatValue_Name_Limit(std::string name, float speed, float limit)
	{
		if (name.empty() || speed < 0.0f)
			return false;

		for (auto& iter : mParametersVec)
		{
			if (!iter)
				continue;
			if (iter->Name == name && iter->Attribute->type == ParameterFLOAT)
			{
				if (*(float*)(iter->Attribute->value) == limit)
					continue;
				else if (*(float*)(iter->Attribute->value) < limit)
				{
					*(float*)(iter->Attribute->value) += speed;
					if ((*(float*)(iter->Attribute->value) > limit))
						*(float*)(iter->Attribute->value) = limit;
				}
				else if ((*(float*)(iter->Attribute->value) > limit))
				{
					*(float*)(iter->Attribute->value) -= speed;
					if (*(float*)(iter->Attribute->value) < limit)
						*(float*)(iter->Attribute->value) = limit;
				}
				return true;
			}
		}
		return false;
	}

	void ControllerComponent::EditorPlayImp()
	{
		if (mLayer)
			mLayer->mFirstStateMachine->ResetmStatePlaying();
		SetRunModel(true);
		mIsPause = false;
	}

	void ControllerComponent::EditorPauseImp(bool pause)
	{
		mIsPause = pause;
	}

	void ControllerComponent::EditorStopImp()
	{
		SetRunModel(false);
		if (mLayer)
			mState = *mLayer->mFirstStateMachine->mStatesVec.begin();
		mIsPause = false;
	}

	bool ControllerComponent::SetParameterBoolValue(int vecIndex, bool value)
	{
		if (mParametersVec.empty() || vecIndex >= (int)mParametersVec.size())
			return false;
		if (!mParametersVec[vecIndex])
			return false;
		if (mParametersVec[vecIndex]->Attribute->ParamtersAttribute::type != Type::ParameterBOOL)
			return false;
		*(bool*)(mParametersVec[vecIndex]->Attribute->value) = value;
		return true;
	}

	bool ControllerComponent::GetParameterBoolValue(int vecIndex)
	{
		if (mParametersVec.empty())
			return false;
		if (vecIndex >= (int)mParametersVec.size())
			return false;
		if (!mParametersVec[vecIndex])
			return false;
		if (mParametersVec[vecIndex]->Attribute->ParamtersAttribute::type != Type::ParameterBOOL)
			return false;
		return *(bool*)(mParametersVec[vecIndex]->Attribute->value);
	}

	bool ControllerComponent::SetInsteadBoolValue(int vecIndex)
	{
		if (mParametersVec.empty() || vecIndex >= (int)mParametersVec.size())
			return false;
		if (!mParametersVec[vecIndex])
			return false;
		if (mParametersVec[vecIndex]->Attribute->ParamtersAttribute::type != Type::ParameterBOOL)
			return false;
		*(bool*)(mParametersVec[vecIndex]->Attribute->value) = !*(bool*)(mParametersVec[vecIndex]->Attribute->value);
		return true;
	}

	bool(*ControllerComponent::ImGuiIsKeydown)(int) = nullptr;
	bool(*ControllerComponent::ImGuiIsKeyup)(int) = nullptr;
	bool ControllerComponent::SetRolePosition(RoleComponent* com, float m_runspeed)
	{
		bool result = false;
		if (!com)
			return result;
		Vector3 pos = com->getPosition();
		Vector3	dir = com->getEulerAngles();
		pos += m_runspeed * dir;
		com->setPosition(pos);
		result = true;
		return result;
	}
	bool ControllerComponent::SetRolePositionToDir(RoleComponent * com, float x, float y, float m_runspeed)
	{
		if (!com)
			return false;
		Vector3 dir = Vector3((0-x), 0.0f, y);
		Vector3 runDir = com->getEulerAngles();
		runDir.y = 0.0f;
		
		Matrix3 relveMatrix;
		Vector3 relveAxis(0.0f, 1.0f, 0.0f);
		//float tempRadian = dir.angleBetween(Vector2(0, 1)).valueRadians();
		dir.TransformCoord(com->GetRole()->getOrientation());

		//Vector3 roleDir = com->GetDirection();
		//float rad = EchoRadianBetween(roleDir, runDir);
		//Vector3 tempVector = runDir.crossProduct(roleDir);


		Vector3 pos = com->getPosition();
		pos += m_runspeed * dir;
		com->setPosition(pos);
		return false;
	}
	Vector3 ControllerComponent::GetDirFromRoleFront(RoleComponent * com, int R)
	{
		float m_yawRadian = 0.2617993877991494f;
		Vector3 cameraDir = Echo::Root::instance()->getMainSceneManager()->getMainCamera()->getDirection();
		cameraDir.y = 0.0f;
		if(R == 0)
			return cameraDir;
		else
		{
			Matrix3 relveMatrix;
			Vector3 relveAxis(0.0f, 1.0f, 0.0f);
			cameraDir.TransformCoord(relveMatrix.SetAxisAngle(relveAxis, m_yawRadian * R));
			return cameraDir;
		}
	}

	bool ControllerComponent::isKeyDown(std::string key)
	{	
		if (ImGuiIsKeydown)
		{
			return ImGuiIsKeydown(Key_KeyboardValue[key]);
		}
		return false;
	}

	bool ControllerComponent::isKeyUp(std::string key)
	{
		if (ImGuiIsKeyup)
		{
			return ImGuiIsKeyup(Key_KeyboardValue[key]);
		}
		return false;
	}

	void ControllerComponent::SendMessageToFun(ProtocolFuncID id, void * p, ProtocolHandle handle)
	{
		ProtocolComponentSystem* pMgr = ProtocolComponentSystem::instance();
		pMgr->callProtocolFunc(id, p, handle);
	}

	void ControllerComponent::SendMessageToFun(Echo::Name name, void * p, ProtocolHandle handle)
	{
		ProtocolComponentSystem* pMgr = ProtocolComponentSystem::instance();
		pMgr->callProtocolFunc(name, p, handle);
	}

	void ControllerComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void ControllerComponent::beginFrame(EventID eventId, ProtocolHandle handle)
	{
		switch (eventId)
		{
		case EID_EveryFrame:
		{
			if (!mIsPlay || mIsPause)
				break;
			bool update = false;
			if (EchoKeyBoard::instance()->GetKeyBoardFromName("w") && !EchoKeyBoard::instance()->GetKeyBoardFromName("s"))
			{
				if (SetParameterFloatValue_Key_Limit(EchoKeyBoard::instance()->GetKeyFromName("w"), 0.2f, EchoKeyBoard::instance()->GetKeyBoardFromName("shift")? 8.0f : 4.0f))
					update = true;
			}
			else if (EchoKeyBoard::instance()->GetKeyBoardFromName("s") && !EchoKeyBoard::instance()->GetKeyBoardFromName("w"))
			{
				if (SetParameterFloatValue_Key_Limit(EchoKeyBoard::instance()->GetKeyFromName("s"), 0.2f, -4.0f))
					update = true;
			}
			else
			{
				if (SetParameterFloatValue_Key_Limit(EchoKeyBoard::instance()->GetKeyFromName("w"))/* && SetParameterFloatBoolValue_Key(EchoKeyBoard::instance()->GetKeyFromName("s"))*/)
					update = true;
			}
			if (EchoKeyBoard::instance()->GetKeyBoardFromName("a") && !EchoKeyBoard::instance()->GetKeyBoardFromName("d"))
			{
				if (SetParameterFloatValue_Key_Limit(EchoKeyBoard::instance()->GetKeyFromName("a"), 0.2f, -4.0f))
					update = true;
			}
			else if (EchoKeyBoard::instance()->GetKeyBoardFromName("d") && !EchoKeyBoard::instance()->GetKeyBoardFromName("a"))
			{
				if (SetParameterFloatValue_Key_Limit(EchoKeyBoard::instance()->GetKeyFromName("d"), 0.2f, 4.0f))
					update = true;
			}
			else
			{
				if (SetParameterFloatValue_Key_Limit(EchoKeyBoard::instance()->GetKeyFromName("a"))/* && SetParameterFloatBoolValue_Key(EchoKeyBoard::instance()->GetKeyFromName("d"))*/)
					update = true;
			}
			//if (EchoKeyBoard::instance()->GetKeyBoard(EchoKeyBoard::instance()->GetKeyFromName("c")))
			if (SetParameterBoolValue_Key(EchoKeyBoard::instance()->GetKeyFromName("c")))
				update = true;
			UpdateBlendtree(update);
			break;
		}
		default:
			break;
		}
	}

	void ControllerComponent::endFrame(EventID, ProtocolHandle handle)
	{
	}

	void ControllerComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}

	void EchoControllerProFuncImp::initProtocolFunction()
	{
		/*auto p = ProtocolComponentSystem::instance();
		REGISTEROMFACTORY(ControllerComponent);*/
	}

	void EchoControllerProFuncImp::uninitProtocolFunction()
	{
	}

	Connection_node::~Connection_node()
	{
		if (ConditionsVec.empty())
			return;
		for (auto& condition : ConditionsVec)
		{
			SAFE_DELETE(condition);
			condition = nullptr;
		}
		ConditionsVec.clear();
		ConditionsVec.shrink_to_fit();
	}

	void Connection_node::SetMet()
	{
		isMet = true;
		for (auto condition : ConditionsVec)
		{
			switch (condition->Term)
			{
			case Terms::None:
				break;
			case Terms::Greater:
			{
				switch (condition->Para->Attribute->type)
				{
				case Type::ParameterFLOAT:
				{
					if (condition->Threshold.F >= *(float*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				case Type::ParameterINT:
				{
					if (condition->Threshold.I > *(int*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				default:
					break;
				}
				break;
			}
			case Terms::Less:
			{
				switch (condition->Para->Attribute->type)
				{
				case Type::ParameterFLOAT:
				{
					if (condition->Threshold.F < *(float*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				case Type::ParameterINT:
				{
					if (condition->Threshold.I < *(int*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				default:
					break;
				}
				break;
			}
			case Terms::Equals:
			{
				switch (condition->Para->Attribute->type)
				{
				case Type::ParameterINT:
				{
					if (condition->Threshold.I != *(int*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				default:
					break;
				}
				break;
			}
			case Terms::NotEquals:
			{
				switch (condition->Para->Attribute->type)
				{
				case Type::ParameterINT:
				{
					if (condition->Threshold.I == *(int*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				default:
					break;
				}
				break;
			}
			case Terms::True:
			{
				switch (condition->Para->Attribute->type)
				{
				case Type::ParameterBOOL:
				{
					condition->Threshold.B = true;
					//bool text = *(bool*)(condition->Para->Attribute->value);
					if (condition->Threshold.B != *(bool*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				default:
					break;
				}
				break;
			}
			case Terms::False:
			{
				switch (condition->Para->Attribute->type)
				{
				case Type::ParameterBOOL:
				{
					condition->Threshold.B = false;
					if (condition->Threshold.B != *(bool*)condition->Para->Attribute->value)
						isMet = false;
					break;
				}
				default:
					break;
				}
				break;
			}
			default:
				break;
			}
		}
	}
	
	Conditions::Conditions(Parameter* parameter)
	{
		parameter->AddConditions(this);
		Para = parameter;
	}

	Parameter::~Parameter()
	{
		for (auto condition : Conditions_List)
		{
			condition->Para = nullptr;
		}
	}

	State::~State()
	{
		for (auto& connection : Connections)
		{
			SAFE_DELETE(connection);
			connection = nullptr;
		}
		Connections.clear();
		Connections.shrink_to_fit();
	}
	void State::DeleteConnection(Connection_node& connection)
	{
		for (auto it = Connections.begin(); it != Connections.end(); ++it)
		{
			if (connection == *(*it))
			{
				Connections.erase(it);
				break;
			}
		}
	}
	bool StateMachine::PlayInMachine()
	{
		if (mStatePlaying == nullptr)
			mStatePlaying = *(mStatesVec.begin());
		if (mStatePlaying->Type == State_Type_Sub_State)
		{
			mStatePlaying->Play();
			for (auto connection : mStatePlaying->Connections)
			{
				if (connection->InputState == mStatePlaying)
					continue;
				connection->SetMet();
				if (connection->isMet)
				{
					RestChildMachines();
					mStatePlaying = static_cast<State*>(connection->InputState);
					if (mStatePlaying->Type == State_Type_Exit)
					{
						mStatePlaying = nullptr;
						return false;
					}
					if (mStatePlaying->Type == State_Type_Nor || mStatePlaying->Type == State_Type_BlendTree)
					{
						mStatePlaying->Play();
						return false;
					}
				}
			}
		}	
		else
		{ 
			for (auto connection : mStatePlaying->Connections)
			{
				if (connection->InputState == mStatePlaying)
					continue;
				connection->SetMet();
				if (connection->isMet)
				{
					RestChildMachines();
					mStatePlaying = static_cast<State*>(connection->InputState);
					if (mStatePlaying->Type == State_Type_Exit)
					{
						mStatePlaying = nullptr;
						return false;
					}
					else
					{
						if (!mStatePlaying->Play())
						{
							mStatePlaying = nullptr;
						}
						break;
					}
				}
			}
		}
		return true;
	}
	void StateMachine::ResetmStatePlaying()
	{
		for (auto& mState : mStatesVec)
		{
			if (mState->Type == State_Type_Sub_State && (mState->StateName.find("(up)")) == (mState->StateName.npos))
				mState->Reset();

		}
		mStatePlaying = nullptr;
	}
	void StateMachine::RestChildMachines()
	{
		for (auto& mState : mStatesVec)
		{
			if (mState->Type == State_Type_Sub_State && (mState->StateName.find("(up)")) == (mState->StateName.npos))
				mState->Reset();

		}
	}
	EchoControllerScript EchoControllerScript::GetControllerCom(const EchoActorScript & pActor, const std::string & name)
	{
		return EchoControllerScript(pActor, name);
	}
	bool EchoControllerScript::SetParameterIntValue(int vecIndex, int value)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterIntValue(vecIndex, value);
		}
		return false;
	}
	bool EchoControllerScript::SetParameterFloatValue(int vecIndex, float value)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterFloatValue(vecIndex, value);
		}
		return false;
	}
	float EchoControllerScript::GetParameterFloatValue(int vecIndex)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->GetParameterFloatValue(vecIndex);
		}
		return 0.0f;
	}
	float EchoControllerScript::GetParameterFloatValue_Name(std::string name)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->GetParameterFloatValue_Name(name);
		}
		return 0.0f;
	}
	bool EchoControllerScript::SetParameterFloatValue_Name(std::string name, float value)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterFloatValue_Name(name, value);
		}
		return false;
	}
	bool EchoControllerScript::SetParameterBoolValue_Key(int key)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterBoolValue_Key(key);
		}
		return false;
	}
	bool EchoControllerScript::SetParameterFloatValue_Key_Limit(int key, float speed, float limit)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterFloatValue_Key_Limit(key, speed, limit);
		}
		return false;
	}
	bool EchoControllerScript::SetParameterFloatValue_Name_Limit(std::string name, float speed, float limit)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterFloatValue_Name_Limit(name, speed, limit);
		}
		return false;
	}
	bool EchoControllerScript::SetParameterBoolValue(int vecIndex, bool value)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetParameterBoolValue(vecIndex, value);
		}
		return false;
	}
	bool EchoControllerScript::GetParameterBoolValue(int vecIndex)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->GetParameterBoolValue(vecIndex);
		}
		return false;
	}
	bool EchoControllerScript::SetInsteadBoolValue(int vecIndex)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetInsteadBoolValue(vecIndex);
		}
		return false;
	}
	bool EchoControllerScript::SetRolePosition(RoleComponent * com, float m_runspeed)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetRolePosition(com, m_runspeed);
		}
		return false;
	}
	bool EchoControllerScript::SetRolePositionToDir(RoleComponent * com, float x, float y, float m_runspeed)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->SetRolePositionToDir(com, x, y, m_runspeed);
		}
		return false;
	}
	Vector3 EchoControllerScript::GetDirFromRoleFront(RoleComponent * com, int R)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->GetDirFromRoleFront(com, R);
		}
		return Vector3();
	}
	void EchoControllerScript::UpdateBlendtree(bool val)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->UpdateBlendtree(val);
		}
	}
	bool EchoControllerScript::isKeyDown(std::string key)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->isKeyDown(key);
		}
		return false;
	}
	bool EchoControllerScript::isKeyUp(std::string key)
	{
		ControllerComponent* controller = static_cast<ControllerComponent*>(m_Ptr.get());
		if (controller) {
			return controller->isKeyUp(key);
		}
		return false;
	}
	bool ParamtersAttribute::ParamtersKeyboard(int key)
	{
		if(keyVec.empty())
			return false;
		for (auto m_key : keyVec)
		{
			if (m_key == key)
				return true;
		}
		return false;
	}
}
