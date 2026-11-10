#ifndef	_EchoMaterialComponent_H__
#define	_EchoMaterialComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

#include "EchoHermiteCurve.h"
#include "EchoHermite.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"

#include "EchoActorEntityComponent.h"
#include "EchoActorRoleComponent.h"

#include "EchoShader.h"


namespace Echo
{
	struct ShadeInfo
	{
		std::string ShadeName;
		std::string PsName;
		std::string VsName;
	};

	struct shaderParamVals
	{
		Echo::EM_UNIFORM_TYPE shaderType;
		uint8 uniformIndex;
		uint8 paramIndex;
		float paramVal;
	};

	struct valueOfShaderParams
	{
		std::vector<shaderParamVals>	shaderParamValsVec;
		std::vector<SFXParam>			FXParamsVec;
		std::vector<std::string>		annotationsMesVec;
	};

	struct MessageOfMaterialComponent
	{
		bool			iscolor = false;
		int				paramIndex = 0;
		int				shaderValueIndex = 0;
		std::string		paramName = "";
		float			val = 0;
		Vector4			color = {0.0f, 0.0f, 0.0f, 0.0f};
	};

	class _EchoComponentExport MaterialComponentInfo;
	class _EchoComponentExport MaterialComponent;

	typedef std::map<String, std::vector<Entity*>>			 ComToEntityMap;
	typedef std::map<Entity*, std::vector<Echo::SubEntity*>> EntityToSubEntityMap;
	typedef std::map<Echo::SubEntity*, valueOfShaderParams>  SubEntityToValueMap;
	class _EchoComponentExport MaterialComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(MaterialComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		/*MaterialComponent(ActorPtr  actor, ActorCOMMoveableType type = UNMOVEABLE);
		~MaterialComponent();*/
	public:
		virtual void							init(ActorComponentInfo* info) override;	// 初始化
		virtual void							connectCom() override;
		virtual void							replay(ActorComponentInfo* info) override;  // 重新设置
		virtual void							bindNode() override;						// 节点绑定
		virtual	void							copyTo(const ActorComponentPtr&  actCom) override;	// 组件复制粘贴
		virtual void                            setTimelineAttribute(std::vector<float>&,const Name&) override;
	public:
		void									AcceptEntityFromCom(EntityComponent* entityCom, std::string comName);
		void									AcceptEntityFromCom(RoleComponent* roleCom, std::string comName);

		Entity*									GetmEntity() { return m_entity; }
		SubEntity*								GetmSubEntityCom() { return m_subEntity; }

		std::string								LoadFileFirst(MaterialComponentInfo* actorinfo);

		void									SetShadeInfoVec();
		void									SetmCurrentEntity(Entity* entityCom) { m_entity = entityCom; }
		void									SetmCurrentEntity(std::string entityName);
		void									SetmCurrentSubEntity(SubEntity* subEntity) { m_subEntity = subEntity; }
		void									SetmCurrentSubEntity(std::string subEntityName);
		void									SetmEntityComName(std::string EntityComName);
		void									SetmRoleComName(std::string RoleComName);

		void									GetSubEntityOfEntity();
		void									ReadMaterialOfSubEntity(SubEntity* subEntity);

		Echo::String							TranslateToChinese(Echo::String &name);
		void									InitalizeChineseWords();

		bool									isResPathChange(std::string UIcomName);

		void									EraseItemOfEntitycomponentMap(Echo::Name entityName);

		void									SetMaterialParam(MessageOfMaterialComponent* message);

		void									ReSetMessageVec();

		void									GetValueFromInfo(SubEntity* subEntity);
		void									SetCustomUniformParamsForSubEntity(SubEntity* subEntity);

		MessageOfMaterialComponent				FindMessageFromParamIndex(int paramIndex);


		MessageOfMaterialComponent				messageHandle;
		std::vector<MessageOfMaterialComponent>	messageHandleVec;
		
		ComToEntityMap							mEntityMap;							//已添加的Entity指针Map	
		Echo::SHADER_PARAMS*					m_curShaderParams[Echo::Shader_Type_Cnt];
		EntityToSubEntityMap					mEntityToSubEntityMap;				//Entity下的subEntity指针
		SubEntityToValueMap						mSubEntityToValueMap;				//每个shader参数的值	,shader的所有参数列表	,每个shader参数需要解析的文件内容

		std::map<Echo::String, Echo::String>	ChineseMap;							//shader英文名与中文名

		std::vector<ShadeInfo>					ShadeInfoVec;

		EntityComponent*						m_entityCom;
		RoleComponent*							m_RoleCom;

	private:
		bool									m_isLoaded;
		Entity*									m_entity;
		SubEntity*								m_subEntity;
	};
	//////////////////////////////////////////////////////////////////////////

	class _EchoComponentExport MaterialComponentInfo : public ActorComponentInfo
	{
	public:
		MaterialComponentInfo() {};
		MaterialComponentInfo(const Name& comType, ActorCOMMoveableType MoveType = UNMOVEABLE);
		virtual ~MaterialComponentInfo();

		virtual void		ExportData(cJSON* jsonNode) override;
		virtual void		ImportData(cJSON* jsonNode) override;
		virtual void		CopyTo(ActorComponentInfo* comInfo) override;
		virtual void		Overlay(ActorComponentInfo* comInfo) override;
		virtual void		OverWrite() override;
	public:


		std::map<String, std::vector<float>> m_ValueMap;			//每个shader参数的值	,shader的所有参数列表	,每个shader参数需要解析的文件内容
		
		std::string m_CurrentEntityName;
		std::string m_CurrentSubEntityName;
		std::string m_EntityComName;
		std::string m_RoleComName;

		EDITOR_REFLECT
		/*DECLARECOMINFO_PTR1(MaterialComponent, std::string, CurrentEntityName, "", SetmCurrentEntity)
		DECLARECOMINFO_PTR1(MaterialComponent, std::string, CurrentSubEntityName, "", SetmCurrentSubEntity)
		DECLARECOMINFO_PTR1(MaterialComponent, std::string, EntityComName, "", SetmEntityComName)
		DECLARECOMINFO_PTR1(MaterialComponent, std::string, RoleComName, "", SetmRoleComName)*/
	};

	class EchoMaterialScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static MaterialComponent*			Script_GetMaterialComponent(ActorPtr  pActor, const std::string& name);
	};

	DEFINECOMFACTORY(MaterialComponent, UNMOVEABLE)
}; // end Echo

#endif 
