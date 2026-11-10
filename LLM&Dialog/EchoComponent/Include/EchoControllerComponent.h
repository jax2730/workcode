//-----------------------------------------------------------------------------
// File:   EchoControllerComponent.h
//
// Author: jianglong
//
// Date:   2022-8-18
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef _EchoControllerComponent_h__
#define _EchoControllerComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoActorRoleComponent.h"
namespace Echo
{
	class TimelineComponent;
	enum ControllerType
	{
		null,
		entity,
		role,		
	};
	enum Type
	{
		ParameterFLOAT,
		ParameterBOOL,
		ParameterINT,
		ParameterTRIGGER
	};
	enum Terms
	{
		None,
		Greater,
		Less,
		Equals,
		NotEquals,
		False,
		True,
		Trigger,
	};
	enum State_Type
	{
		State_Type_Nor = 0,
		State_Type_Entry,
		State_Type_Exit,
		State_Type_Sub_State,
		State_Type_BlendTree
	};

	static std::map<std::string, int> Key_KeyboardValue =
	{
		{" ",0},
		{"a",546},{"b",547},{"c",548},{"d",549},{"e",550},{"f",551},{"g",552},{"h",553},{"i",554},{"j",555},
		{"k",556},{"l",557},{"m",558},{"n",559},{"o",560},{"p",561},{"q",562},{"r",563},{"s",564},{"t",565},
		{"u",566},{"v",567},{"w",568},{"x",569},{"y",570},{"z",571}
	};

	struct Conditions;
	struct StateMachineFileData;
	struct StateFileData;
	struct ParameterFileData
	{
		Type		parameterType;
		std::string	parameterName;
		float		value_f = 0.0f;
		int			value_i = 0;
		bool		value_b = false;
		std::vector<int> keyVec = { 0,0 };
	};

	struct ConditionFileData
	{
		std::string ParamName;
		Terms		Term;
		float		f;
		bool		b;
		int			i;
	};
	struct ConnectionFileData
	{
		std::string						InputState;
		std::string						OutputState;
		std::string						InputSlot;
		std::string						OutputSlot;
		std::vector<ConditionFileData>	ConditionVec;
	};

	struct StateMachineFileData
	{
		std::string							name;
		std::vector<StateFileData*>			stateDataVec;
	};

	struct StateFileData
	{
		Echo::Name							name;
		Echo::Name							animationName_1;
		Echo::Name							animationName_2;
		State_Type							type;
		Vector2								pos;
		bool								isDefaultState;
		std::vector<ConnectionFileData>		connections;
		StateMachineFileData				sub_StateMachine;
	};

	struct LayerFileData
	{
		StateMachineFileData				firstMachineData;
	};

	class _EchoComponentExport ParamtersAttribute
	{
	public:
		ParamtersAttribute(Type TYPE) { type = TYPE; };
		~ParamtersAttribute() {};
		virtual void ShowUI() = 0;
		bool	ParamtersKeyboard(int key);
	public:
		Type	type;
		void*	value;
		std::vector<int>	keyVec = { 0,0 };
	};

	struct _EchoComponentExport Parameter
	{
	public:
		std::string					Name;
		ParamtersAttribute*			Attribute;

		std::vector<Conditions*>	Conditions_List;

		void	AddConditions(Conditions* condition)
		{
			Conditions_List.push_back(condition);
		}
		Parameter(std::string name, ParamtersAttribute* attribute)
		{
			Name = name;
			Attribute = attribute;
		}
		~Parameter();
	};
	union ConditionValue
	{
		float	F = 0.0f;
		bool	B;
		int		I;	
	};
	/// A structure defining a connection between four slots of two nodes.
	struct _EchoComponentExport Connection_node
	{
		bool  isMet = true;
		std::vector<Conditions*>	ConditionsVec;
		/// `id` that was passed to BeginNode() of input node.
		void* InputState = nullptr;
		/// Descriptor of input slot.
		std::string InputSlot = "";
		/// `id` that was passed to BeginNode() of output node.
		void* OutputState = nullptr;
		/// Descriptor of output slot.
		std::string OutputSlot = "";
		bool operator==(const Connection_node& other) const
		{
			return InputState == other.InputState &&
				InputSlot == other.InputSlot &&
				OutputState == other.OutputState &&
				OutputSlot == other.OutputSlot;
		}

		bool operator!=(const Connection_node& other) const
		{
			return !operator ==(other);
		}
		Connection_node() {};
		~Connection_node();
	public:
		void	SetMet();
	};

	class _EchoComponentExport State
	{
	public:
		std::string      		StateName = "";
		Echo::Name				AnimationName_1;
		Echo::Name				AnimationName_2;
		State_Type				Type;
		bool					IsDefaultState = false;
		float*					Position_x;
		float*					Position_y;
	public:
		
	public:
		//TimelineComponent*		TimelineCom = nullptr;
		/// List of node connections.
		std::vector<Connection_node*> Connections{};
	public:
		explicit State(State_Type type) {
			Type = type; 
			AnimationName_1 = "";
			AnimationName_2 = "";
		};
		virtual ~State();
		/// Deletes connection from this node.
		void DeleteConnection(Connection_node& connection);
	public:
		virtual bool			Play() { return true; };
		virtual void			Reset() {};
		virtual bool			IsPlayStop() {return false;};
	};

	struct _EchoComponentExport Conditions
	{
		Parameter*		Para;
		Terms			Term;
		ConditionValue  Threshold;
		Conditions(Parameter* parameter);
		~Conditions() {};
	};

	class _EchoComponentExport StateMachine 
	{
	public:
		Echo::Name					mName;
		State*						mStatePlaying = nullptr;
		StateMachine*				mUpStateMachine;
		std::vector<State*>			mStatesVec;
		size_t						mIndex = 0;
		StateMachine(Echo::Name name, StateMachine* upStateMachine = nullptr)
		{
			mUpStateMachine = upStateMachine;
			mName = name;
		}
		~StateMachine()
		{
			for (auto& state : mStatesVec)
			{
				SAFE_DELETE(state);
				state = nullptr;
			}
			mStatesVec.clear();
			mStatesVec.shrink_to_fit();
		}
		bool	PlayInMachine();
		void	ResetmStatePlaying();
		void	RestChildMachines();
	};

	struct _EchoComponentExport Layer
	{
		StateMachine*		mFirstStateMachine;

		Layer(StateMachine* firstStateMachine)
		{
			mFirstStateMachine = firstStateMachine;
		}
		
		~Layer()
		{
			delete mFirstStateMachine;
			mFirstStateMachine = nullptr;
		}
	};
	struct BehaviorFildData : public StateFileData
	{
	public:
		float			pos_x;
		float			pos_y;
		float			weight;
		int				animationIndex1;
		int				animationIndex2;
	};
	struct BlendTreeNodeFileData : public StateFileData
	{
	public:
		std::string	 						pos_x;
		std::string							pos_y;
		float								weight;
		Vector2								childblendtree_pos;
		std::vector<BlendTreeNodeFileData>	BlendtreeList;
		std::vector<BehaviorFildData>		BehaviorList;
	};
	struct BlendTreeFileData : public StateFileData
	{
	public:
		std::string				roleCom;
		BlendTreeNodeFileData	blendTree;
		bool					windowOpen;
	};

	class ControllerComponentInfo;
	//=============================================================================
	class _EchoComponentExport ControllerComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(ControllerComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		/*ControllerComponent(Actor* pActor, ActorCOMMoveableType isMoveable = UNMOVEABLE);
		~ControllerComponent();*/

	public:
		void								    init(ActorComponentInfo* info) override;
		virtual void							copyTo(const Echo::ActorComponentPtr& p_actor_component) override;

		void									registerTickTimeBegin() override;
		virtual void							beginFrame(EventID, ProtocolHandle handle) override;
		virtual void							endFrame(EventID, ProtocolHandle handle) override;
		virtual void							preUpdateAnimation(EventID, ProtocolHandle handle) override {}
		virtual void							postUpdateAnimation(EventID, ProtocolHandle handle) override {}
		virtual void							preUpdateScene(EventID, ProtocolHandle handle) override {}
		virtual void							postUpdateScene(EventID, ProtocolHandle handle) override {}
		virtual void							SendMessageToFun(ProtocolFuncID id, void* p, ProtocolHandle handle);
		virtual void							SendMessageToFun(Echo::Name name, void* p, ProtocolHandle handle);
		virtual	void							setActor(const ActorPtr& pActor) override;
		//ImGui Set Value
		virtual void							preRenderScene(EventID, ProtocolHandle handle) override;
		//ImGui Get Value
		virtual void							postRenderScene(EventID, ProtocolHandle handle) override;

	public:
		void									CreateControllerFile(std::string& filePath);
		void									CreateStateDataItem(Echo::cJSON* item, std::vector<StateFileData*> stateFileData);
		void									CreateBlendTreeDataItem(Echo::cJSON* iter, BlendTreeNodeFileData& blendTreeData);
		void									ReadControllerFile(std::string& filePath);
		void									ReadStateDataItem(cJSON* stateVec, std::vector<StateFileData*>& stateDataVec);
		void									ReadBlendTreeDataItem(cJSON* blendtreeVec, BlendTreeNodeFileData& blendTreeData);

		void									InitializemLayer() { mLayer = *(mLayerVec.begin()); };
		void									InitializemPath() { mStateMachinePath = mLayer->mFirstStateMachine->mName.c_str(); };
		void									SetRunModel(bool model) { mRun = model; };
		void									SetControllerType(ControllerType type) { mControllerType = type; };
		void									SetmControllerFilePath(std::string filePath);
		void									SaveDataToControllerFileStruct();
		void									SavestateDataVec(StateMachineFileData stateMachineInfoData, std::vector<State*> statesVec);

		void									Transiting();
		void									PlayInStateMachine(StateMachine* stateMachine);
		void									AddLayer(StateMachine* firstStateMachine);
		void									AddParameterVec(std::string& ParameterName, ParamtersAttribute* Attribute);
		void									ClearLayers();
		void									ClearParmeterVec();
		void									SetParameterName(std::string& ParameterName); 

		void									EnterMachinePath(StateMachine* orderMachine);
		void									ReturnMachinePath();
		
		bool									IsChangeParameterName(std::string name);

		//void									ControllerKeyDown(int key);
		//void									ControllerKeyUp(int key);

		/////////////////////////////////script////////////////////////////////////////////////////////////////
		bool									SetParameterIntValue(int vecIndex, int value);
		bool									SetParameterFloatValue(int vecIndex, float value);
		float									GetParameterFloatValue(int vecIndex);
		float									GetParameterFloatValue_Name(std::string name);
		bool									SetParameterFloatValue_Name(std::string name, float value);
		bool									SetParameterBoolValue_Key(int key);
		bool									SetParameterFloatValue_Key_Limit(int key, float speed = 0.4f/* >= 0 */,float limit = 0.0f);
		bool									SetParameterFloatValue_Name_Limit(std::string name, float speed = 0.4f/* >= 0 */, float limit = 0.0f);
		bool									SetParameterBoolValue(int vecIndex, bool value);
		bool									GetParameterBoolValue(int vecIndex);
		bool									SetInsteadBoolValue(int vecIndex);

		bool									SetRolePosition(RoleComponent* com, float m_runspeed);
		bool									SetRolePositionToDir(RoleComponent* com, float x, float y, float m_runspeed);
		Vector3									GetDirFromRoleFront(RoleComponent* com, int R);

		void									UpdateBlendtree(bool val) { mIsUpdateBlendtree = val; };
		//脚本可以调用的键盘检测函数
		static bool								isKeyDown(std::string key);
		static bool								isKeyUp(std::string key);
		static bool								(*ImGuiIsKeydown)(int);
		static bool								(*ImGuiIsKeyup)(int);
		
	protected:
		virtual void					EditorPlayImp() override;
		virtual void					EditorPauseImp(bool pause) override;
		virtual void					EditorStopImp() override;

		//bool									
	public:
		//---保存在.controller文件中的数据，需要解析才能成为StateList与ParamtersVec---//
		std::vector<ParameterFileData>			mParametersDataVec;
		std::vector<LayerFileData>				mLayerDataVec;
		//----------------------------------------------------------------------------//
		std::vector<Layer*>						mLayerVec;
		std::vector<Parameter*>					mParametersVec;
		//----------------------------------------------------------------------------//

		std::string								mControllerFilePath;
		ControllerType							mControllerType;
	
		std::string								mStateMachinePath;
		//ControllerComponentInfo*                ControllerComInfo;	
		Layer*									mLayer;
		StateMachine*							mStateMachine;
		State*									mState;
		
		bool									mIsPlay = false;
		bool									mIsPause = false;
		bool									mRun;
		bool									mIsUpdateBlendtree = false;

	private:

	};

	//=============================================================================
	class _EchoComponentExport ControllerComponentInfo : public ActorComponentInfo
	{
	public:
		ControllerComponentInfo() {};
		ControllerComponentInfo(const Name& name, ActorCOMMoveableType moveable = UNMOVEABLE);
		virtual ~ControllerComponentInfo();
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo * info) override;
		virtual void Overlay(ActorComponentInfo * comInfo) override;
		virtual void OverWrite() override;

	public:
		//DECLARECOMINFO_PTR1(ControllerComponent, ControllerType, controllerType, ControllerType(0), SetControllerType)
		//DECLARECOMINFO_PTR1(ControllerComponent, std::string, filePath, "", SetmControllerFilePath)
		int				GetcontrollerType() { return m_controllerType; };
		void			SetcontrollerType(int val) { m_controllerType = val; };
		std::string		GetfilePath() { return m_filePath; };
		void			SetfilePath(std::string path) { m_filePath = path; };
		int				m_controllerType = 0;
		std::string		m_filePath = "";
		
		EDITOR_REFLECT
	};
	
	class _EchoComponentExport EchoControllerProFuncImp
	{
	public:
	//-----------------------------------------------------------------------------
		static void								initProtocolFunction();
		static void								uninitProtocolFunction();
	};

	class EchoControllerScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoControllerScript() = default;
		EchoControllerScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {}

	public:
		static EchoControllerScript			GetControllerCom(const EchoActorScript& pActor, const std::string& name);

	public:
		bool									SetParameterIntValue(int vecIndex, int value);
		bool									SetParameterFloatValue(int vecIndex, float value);
		float									GetParameterFloatValue(int vecIndex);
		float									GetParameterFloatValue_Name(std::string name);
		bool									SetParameterFloatValue_Name(std::string name, float value);
		bool									SetParameterBoolValue_Key(int key);
		bool									SetParameterFloatValue_Key_Limit(int key, float speed = 0.4f/* >= 0 */, float limit = 0.0f);
		bool									SetParameterFloatValue_Name_Limit(std::string name, float speed = 0.4f/* >= 0 */, float limit = 0.0f);
		bool									SetParameterBoolValue(int vecIndex, bool value);
		bool									GetParameterBoolValue(int vecIndex);
		bool									SetInsteadBoolValue(int vecIndex);

		bool									SetRolePosition(RoleComponent* com, float m_runspeed);
		bool									SetRolePositionToDir(RoleComponent* com, float x, float y, float m_runspeed);
		Vector3									GetDirFromRoleFront(RoleComponent* com, int R);

		void									UpdateBlendtree(bool val);
		//脚本可以调用的键盘检测函数
		bool									isKeyDown(std::string key);
		bool									isKeyUp(std::string key);
	};
	//控制器组件工厂类
	DEFINECOMFACTORY(ControllerComponent, UNMOVEABLE);
}
// namespace Echo
#endif 
