#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoCSGUtil.h"
#include "EchoPhysXScene.h"

namespace Echo
{
	class _EchoComponentExport CSGComponent :public NodeActorComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(CSGComponent)

		void init(ActorComponentInfo* info) override;

		/// <summary>
		/// 客户端调用这个保存mesh资产，需要按顺序传入。prsData和geometry保持一致。
		/// </summary>
		/// <param name="prsData">传入Geometry的local pos,eulerangle,scale</param>
		/// <param name="geometrys">Area使用的EchoGeometry</param>
		/// <param name="destMeshPath">存储路径</param>
		/// <param name="templateMatPath">模板材质路径</param>
		/// <returns></returns>
		bool generateMeshFromArea(vector<Vector3>::type& prsData, vector<EchoGeometry*>::type& geometrys,Echo::String& destMeshPath,const Echo::String& templateMatPath = "");

		/*bool generateMeshFromArea(vector<Vector3>::type& prsData, vector<EchoGeometry*>::type& geometrys, Echo::String& destMeshPath, const Echo::String& outerMatPath = "", const Echo::String& innerMatPath = "");*/
		bool generateMeshFromArea(vector<Vector3>::type& prsData, vector<EchoGeometry*>::type& geometrys, Echo::String& destMeshPath, const Echo::String& outerMatPath, const Echo::String& innerMatPath);

		//-------test Union------
		void mergeBrush();
		//-------test Union------
	};

	class _EchoComponentExport CSGComponentInfo : public ActorComponentInfo
	{
		friend class CSGComponent;
	public:
		CSGComponentInfo() {}
		CSGComponentInfo(const Name& comType, ActorCOMMoveableType moveType);

		virtual ~CSGComponentInfo();

		_DECLCOMINFO_VIRFUN

	public:
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(CSGComponent, EDITOR_MOVEABLE)
}
