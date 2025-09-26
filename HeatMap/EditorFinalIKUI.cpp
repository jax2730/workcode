#include "EchoEditorStableHeaders.h"

#include "RoleEditorUI/EditorFinalIKUI.h"

#include "EchoEditorRoot.h"

#include "EchoSkeletonInstance.h"

#include "EchoBellSystem.h"

#include "EchoBellResourceManager.h"

#include "EchoPhysicsManager.h"

#include "EchoRigidBodyComponent.h"

#include "EchoCharactorControllerComponent.h"

#include "EchoJointComponent.h"

#include "EchoRigidBodyGroupComponent.h"

//#include "Interactor/EchoInteractorManager.h"

//#include "Interactor/EchoInteractor.h"

//#include "Interactor/EchoInteractorObject.h"

//#include "Interactor/EchoInteractorTarget.h"

#include "FileTools/EchoEditorFileLoadSaveWindow.h"

#include "FileTools/EchoEditorFileToolsManager.h"

#include "ActorEditorUI/EditorActorUIComponent.h"

#include <io.h>

#include "EchoPhysXMeshSerializer.h"

#include "EchoMeshMgr.h"

#include "EchoResourceFolderManager.h"

#include "EchoHelperObj.h"

#include "EchoPhysXManager.h"

#include "Actor/EchoActorInfoManager.h"

#include "EchoColliderComponent.h"

#include "EchoBoxColliderComponent.h"

#include "EchoPhysXScene.h"

#include "Actor/EchoNodeComponent.h"

#include "EchoPhysicsCore.h"

#include "EchoPhysicsPagedLayer.h"

#include "EchoActorEntityComponent.h"

#include "ActorEditorUI/EditorComponentCommonFunctionsUI.h"

#include "EchoWorldSystem.h"

#include "EchoSphericalTerrainManager.h"

#include "EchoSphericalTerrainComponent.h"

#include "EchoSubMesh.h"

#include "Role/EchoRoleEntityComponent.h"

#include "EchoSubEntity.h"

#include "EchoManualObject.h"

#include "EchoSphericalTerrianQuadTreeNode.h"

#include "EchoOAESystem.h"

#include "EchoPolyhedronGravity.h"

#include "EchoSphericalTerrainResource.h"

#include "EchoBiomeGenObjectLibrary.h"

#include <fstream>

#include <sstream>

#include <iostream>

#include "EchoPCGClutter.h"



#define TEXT_BUFFER_MAX_PATH 500



namespace Echo 

{

	namespace EditorFinalIK {



		const ImVec4 NodeSelectButtonColor(0.6f, 0.f, 0.f, 1.0f);

		const ImVec4 LimbSelectButtonColor(0.f, 0.6f, 0.f, 1.0f);

		const ImVec4 FBBIKSelectButtonColor(0.f, 0.f, 0.6f, 1.0f);

		const ImVec4 WorldNodeSelectButtonColor(0.5f, 0.f, 0.5f, 1.0f);

		const ImVec4 LocatorSelectButtonColor(0.5f, 0.5f, 0.f, 1.0f);

		const ImVec4 DefaultButtonColor(0.1137f, 0.1843f, 0.2863f, 1.0f);



		const ImVec4 LockRoleButtonColor(0.6f, 0.f, 0.f, 1.0f);

		const ImVec4 LockRoleButtonHoveredColor(1.0f, 0.f, 0.f, 1.0f);



		const ImVec4 DisableStateColor(0.38f, 0.38f, 0.38f, 1.0f);



		const std::vector<const char*> EFIKGroundingLayers::layerTypeLabels = { "Nothing", "Everything", "Default", "TransparentFX", "Ignore Raycast", "Water", "UI" };

		const std::vector<const char*> EFIKGroundingUI::qualityTypeLabels = { "Fastest","Simple","Best" };



		const std::vector<const char*> EFIKLimbIKUI::AvatarIKGoalTypeLabels = { "Left Foot", "Right Foot", "Left Hand", "Right Hand" };

		const std::vector<const char*> EFIKLimbIKUI::BendModifierTypeLabels = { "Animation", "Target", "Parent", "Arm", "Goal" };



		const std::map<UINT32, const char*> gUIErrorInfoMap = { 

			{ UIErrorType_ParametersAreNotFullySet,U8("自身参数未设置完全")},

			{ UIErrorType_ChildrenParametersAreNotFullySet, U8("子项参数未设置完全")}

		};



		const char* GetUIErrorInfo(UINT32 type) {

			if (type < UIErrorType_Number) {

				return gUIErrorInfoMap.find(type)->second;

			}

			return nullptr;

		}



		const std::vector<const char*> EFIKSpineEffectorUI::EffectorEnumLabel = {

			"Body",

			"LeftShoulder",

			"RightShoulder",

			"LeftThigh",

			"RightThigh",

			"LeftHand",

			"RightHand",

			"LeftFoot",

			"RightFoot"

		};



		const std::vector<const char*>  EFIKFBIKChainUI::SmoothingEnumLabel = {

			"None",

			"Exponential",

			"Cubic"

		};





		std::vector<int> EnumToInt_layer = { 0, INT_MAX,1,2,4,16,32 };



		PromptDialog UseWhenNotSet("Error", U8("参数未完全设置，是否Use Component"));

		PromptDialog CreateObjectNameNULL("Error", U8("名字不能为空"));

		PromptDialog CreateObjectNameExist("Error", U8("名字已存在"));

		Echo::Name NullName("");

	

	};



	//==============================================================================================================================================



	namespace EditorFinalIK {

	

		void IKSliderFloat(const char* label, float* data, float min = 0.0f, float max = 1.0f, const char* format = "%.3f") {



			ImGui::SetNextItemWidth(std::max(ImGui::GetCurrentWindow()->DC.ItemWidth - 57.0f, 10.0f));

			ImGui::SliderFloat(("##" + std::string(label)).c_str(), data, min, max, format);

			ImGui::SameLine();

			ImGui::SetNextItemWidth(50);

			if (ImGui::InputFloat(label, data, 0.0f, 0.0f, format)) {

				if (*data < min) {

					*data = min;

				}

				else if (*data > max) {

					*data = max;

				}

			}



		}



		void IKSliderInt(const char* label, int* data, int min, int max) {



			if (ImGui::InputInt(label, data, 1, 100)) {

				if (*data < min) {

					*data = min;

				}

				else if (*data > max) {

					*data = max;

				}

			}

		}



		void IKDragFloat(const char* label, float* data, float speed, float min, float max, const char* format = "%.3f") {

			if (min == FLT_MIN) {

				min = -FLT_MAX / INT_MAX;

			}

			if (max == FLT_MAX) {

				max = FLT_MAX / INT_MAX;

			}

			ImGui::DragFloat(label, data, speed, min, max, format);

			if (*data < min) {

				*data = min;

			}

			else if (*data > max) {

				*data = max;

			}

		}



		void IKDragInt(const char* label, int* data, float speed, int min, int max, const char* format = "%d") {



			ImGui::DragInt(label, data, speed, min, max, format);

			if (*data < min) {

				*data = min;

			}

			else if (*data > max) {

				*data = max;

			}

		}



		PromptDialog::PromptDialog(const char* label, const char* text) : mLabel(label), mText(text) {

			mWidth = (float)std::strlen(text) * 5.0f;

		}

		PromptDialog::PromptDialog() {  }

		PromptDialog::_PromptDialog_ClickType PromptDialog::Show(const char* text) {

			if (!ImGui::IsPopupOpen(mLabel)) {

				ImGui::OpenPopup(mLabel);

			}

			_PromptDialog_ClickType res = _PromptDialog_ClickType::NONE;

			if (ImGui::BeginPopupModal(mLabel, NULL, ImGuiWindowFlags_AlwaysAutoResize))

			{

				bool IsButton = false;

				float width = mWidth;

				if (text) {

					width = (float)std::strlen(text) * 5.0f;

				}

				ImGui::SetNextItemWidth(width);

				if (text) {

					ImGui::Text(text);

				}

				else {

					ImGui::Text(mText);

				}

				if (ImGui::ButtonEx(U8("确定##PromptDialog"), ImVec2(width * 0.5f, 0.f))) {

					ImGui::CloseCurrentPopup();

					res = _PromptDialog_ClickType::OK;

				}

				ImGui::SameLine();

				if (ImGui::ButtonEx(U8("取消##PromptDialog"), ImVec2(width * 0.5f, 0.f))) {

					ImGui::CloseCurrentPopup();

					res = _PromptDialog_ClickType::CANCEL;

				}

				ImGui::EndPopup();

			}

			return res;

		}

	};



	//==============================================================================================================================================



	//BaseClass

	namespace EditorFinalIK {



		ChildUIBase::ChildUIBase() : m_ShowType(UIShowType::None) { SetLabel("None"); }

		ChildUIBase::ChildUIBase(ChildUIBase& obj) { *this = obj; }

		ChildUIBase::~ChildUIBase() { delete[] mName; mName = nullptr; }



		bool ChildUIBase::ShowGui(UIShowType ShowType, float Width) {

			if (ShowType == Default) {

				ShowType = m_ShowType;

			}

			float width = 1.0f;

			bool IsSetWidth = true;

			if (Width > 1) {

				width = Width;

			}

			else if (width > 0) {

				width = ImGui::GetWindowWidth() * Width;

			}

			else {

				IsSetWidth = false;

			}

			switch (ShowType) {

			case TreeNode: {

				if (ImGui::TreeNode(mName)) {

					ExecuteShow(width, IsSetWidth);

					ImGui::TreePop();

				}

				return false;

			}

			case TreeNodeEx_Leaf: {

				if (ImGui::TreeNodeEx(mName, ImGuiTreeNodeFlags_Leaf)) {

					ExecuteShow(width, IsSetWidth);

					ImGui::TreePop();

				}

				return false;

			}

			case TreeNodeEx_OpenOnArrow: {

				if (ImGui::TreeNodeEx(mName, ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

					ExecuteShow(width, IsSetWidth);

					ImGui::TreePop();

				}

				return false;

			}

			case TreeNodeEx_Del: {

				if (ImGui::TreeNodeEx(mName, ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

					ImGui::SameLine(width);

					bool IsDel = ImGui::ButtonEx("Del##DeleteMe", ImVec2(width, 0));

					ExecuteShow(width, IsSetWidth);

					ImGui::TreePop();

					return IsDel;

				}

				return false;

			}

			case CollapsingHeader: {

				if (ImGui::CollapsingHeader(mName)) {

					ImVec2 size = ImGui::GetWindowSize();

					ImVec2 pos = ImGui::GetWindowPos();

					ImGui::SetWindowPos(ImVec2(pos + ImVec2(size.x*0.9f, 0.f)));

					bool IsDel = ImGui::ButtonEx("Del##DeleteMe", ImVec2(size.x * 0.1f, 0.f));

					ImGui::SetWindowSize(size);

					ImGui::SetWindowPos(pos);

					ExecuteShow(width, IsSetWidth);

					return IsDel;

				}

				return false;

			}

			case Begin: {

				if (ImGui::Begin(mName)) {

					ExecuteShow(width, IsSetWidth);

					ImGui::End();

				}

				return false;

			}

			case BeginChild: {

				if (!ImGui::BeginChild(mName)) {

					ImGui::EndChild();

				}

				else {

					ExecuteShow(width, IsSetWidth);

					ImGui::EndChild();

				}

				return false;

			}

			case BeginPopup: {

				if (ImGui::BeginPopup(mName)) {

					ExecuteShow(width, IsSetWidth);

					ImGui::EndPopup();

				}

				return false;

			}

			default: {

				ExecuteShow(width, IsSetWidth);

				return false;

			}

			}

			return false;

		}

		void ChildUIBase::ExecuteShow(float width, bool IsSetWidth) {

			if (IsSetWidth) {

				ImGui::PushItemWidth(width);

				Show();

				ImGui::PopItemWidth();

			}

			else {

				Show();

			}

		}

		ChildUIBase& ChildUIBase::operator=(const ChildUIBase& obj) {

			if (this != &obj) {

				SetLabel(obj.mName);

			}

			return *this;

		}

		void ChildUIBase::SetLabel(const char* label) {

			if (mName != nullptr)

				delete[] mName;

			int len = (int)strlen(label);

			mName = new char[len + 1];

			strcpy_s(mName, len + 1, label);

		}

		void ChildUIBase::SetLabel(const std::string& label) {

			if (mName != nullptr)

				delete[] mName;

			int len = (int)strlen(label.c_str());

			mName = new char[len + 1];

			strcpy_s(mName, len + 1, label.c_str());

		}

		void ChildUIBase::SetLabel(const Name& label) {

			if (mName != nullptr)

				delete[] mName;

			int len = (int)strlen(label.c_str());

			mName = new char[len + 1];

			strcpy_s(mName, len + 1, label.c_str());

		}

		const char* ChildUIBase::GetLabel() {

			return mName;

		}

		void ChildUIBase::SetShowType(UIShowType type) {

			m_ShowType = type;

		}

		UIShowType ChildUIBase::GetShowType() {

			return m_ShowType;

		}



		template<typename T>

		void EditorVariGroupUI<T>::Reset() {

			if (mCount > mGroup.size()) {

				int OldSize = (int)mGroup.size();

				mGroup.resize(mCount);

				for (int i = OldSize; i != mGroup.size(); ++i) {

					mGroup[i] = new T();

					mGroup[i]->SetLabel("Element" + std::to_string(i));

					mGroup[i]->SetShowType(elementType);

					mGroup[i]->Init();

				}

			}

			else {

				for (int i = mCount; i != mGroup.size(); ++i) {

					delete mGroup[i];

				}

				mGroup.resize(mCount);

			}

		}

		template<typename T>

		void EditorVariGroupUI<T>::Clear() {

			mCount = 0;

			Reset();

		}

		template<typename T>

		void EditorVariGroupUI<T>::Resize(int count) {

			mCount = count;

			Reset();

		}

		template<typename T>

		int EditorVariGroupUI<T>::size() {

			return mCount;

		}

		template<typename T>

		T* EditorVariGroupUI<T>::Get(int n) {

			if (n < mCount) {

				return mGroup[n];

			}

			return nullptr;

		}

		template<typename T>

		EditorVariGroupUI<T>::EditorVariGroupUI() { Init(); }

		template<typename T>

		EditorVariGroupUI<T>::~EditorVariGroupUI() { Clear(); }

		template<typename T>

		void EditorVariGroupUI<T>::Init() { SetShowType(UIShowType::TreeNode); }

		template<typename T>

		void EditorVariGroupUI<T>::Load()  {

			/*for (auto item : mGroup) {

				item->Load();

			}*/

		}

		template<typename T>

		void EditorVariGroupUI<T>::Show()  {



			bool IsDisable = CURRENTALLUI->GetIsDisable();

			if (!IsDisable) {

				ImGui::InputInt("Number", &mCount);

				mCount = abs(mCount);

				Reset();

			}

			else {

				ImGui::InputInt("Number", &mCount, 1, 100, ImGuiInputTextFlags_ReadOnly);

			}

			for (int i = 0; i < mGroup.size(); i++)

			{

				T* item = mGroup[i];

				ImGui::PushID(i);

				item->ShowGui();

				ImGui::PopID();

			}



		}

		template<typename T>

		void EditorVariGroupUI<T>::Update() {

			/*for (auto item : mGroup) {

				item->Update();

			}*/

		};



		template<typename IKCLASS, typename INFOCLASS>

		void EFIKComponentBase<IKCLASS,INFOCLASS>::UseComponent(bool IsUse) {

			IsUseComponent = IsUse;

			if (this->m_pData) {

				if (IsUse) {

					this->m_pData->enable();

				}

				else {

					this->m_pData->disable();

				}

			}

		}



		template<typename IKCLASS, typename INFOCLASS>

		void EFIKComponentBase<IKCLASS, INFOCLASS>::ResetComponent(bool IsUse) {

			PreResetComponent(IsUse);

			if (IsUse) {

// 				RecursionUseComponent(IsUse);

				

				Name name(this->GetLabel());



				IKCLASS* ikclass = (IKCLASS*)EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->GetIKComponent(name);

				if (!ikclass) {

					INFOCLASS* pInfo = new INFOCLASS();

					this->WriteInfo(pInfo);

					pInfo->SetName(name);

					ikclass = (IKCLASS*)EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->CreateIKComponent(name, pInfo);

					_ECHO_DELETE(pInfo);

				}

				this->SetData(ikclass);

				UseComponent(true);

// 				AdjustComponent(IsUse);

			}

			else {

// 				AdjustComponent(IsUse);

				UseComponent(false);

				Name name(this->GetLabel());

				if (IsDestroyIKInNotUse) EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->destroyIK(name);

				this->SetData(nullptr);

// 				RecursionUseComponent(IsUse);

			}

			PostResetComponent(IsUse);

		}



		template<typename IKCLASS, typename INFOCLASS>

		void EFIKComponentBase<IKCLASS, INFOCLASS>::Show() {

			ControlComponent();

			CURRENTALLUI->PushIsDisable(IsUseComponent);

			PureShow();

			CURRENTALLUI->PopIsDisable();

		}



		template<typename IKCLASS, typename INFOCLASS>

		EFIKComponentBase<IKCLASS, INFOCLASS>::EFIKComponentBase() {

			this->SetShowType(UIShowType::TreeNodeEx_Del);

			EditorFinalIKUI::instance()->GetCurUI()->AddPtrToPool(this);

		}

		/*template<typename IKCLASS, typename INFOCLASS>

		EFIKComponentBase<IKCLASS, INFOCLASS>::EFIKComponentBase(RoleObject* pRoleObject) {

			m_pData = new IKCLASS(pRoleObject->getRoleIKComponent());

			assert(m_pData != nullptr);

			SetShowType(UIShowType::TreeNodeEx_Del);

		}*/

		template<typename IKCLASS, typename INFOCLASS>

		EFIKComponentBase<IKCLASS, INFOCLASS>::~EFIKComponentBase() {



			UseComponent(false);

			EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->destroyIK(Name(this->GetLabel()));

			this->m_pData = nullptr;

			EditorFinalIKUI::instance()->GetCurUI()->RemovePtrFromPool(this);

		}

		template<typename IKCLASS, typename INFOCLASS>

		void EFIKComponentBase<IKCLASS, INFOCLASS>::ControlComponent() {

			if (ImGui::Checkbox("Use Component", &IsUseComponent)) {

				IsJudgeUse = true;

			}

			ImGui::SameLine(); HelpMarker("Whether to use the Component");

			UpdateComponent();

		}

		template<typename IKCLASS, typename INFOCLASS>

		void EFIKComponentBase<IKCLASS, INFOCLASS>::CallUseComponent(bool IsUse) {

			IsDestroyIKInNotUse = !IsUse;

			ResetComponent(IsUse);

		}

		template<typename IKCLASS, typename INFOCLASS>

		void EFIKComponentBase<IKCLASS, INFOCLASS>::UpdateComponent() {



			if (IsJudgeUse) {

				if (!IsUseComponent) {

					IsJudgeUse = false;

				}

				else {

					UINT32 errType = JudgeCanUseComponent();

					if (errType == UIErrorType_NoError) {

						IsJudgeUse = false;

					}

					else {

						PromptDialog::_PromptDialog_ClickType res = UseWhenNotSet.Show(GetUIErrorInfo(errType));

						if (res != PromptDialog::_PromptDialog_ClickType::NONE) {

							IsJudgeUse = false;

							IsUseComponent = false;

						}

					}

				}

				return;

			}

			if (IsUseComponent) {

				if (IsInitComponent) {

					this->Update();

				}

				else {

					ResetComponent(true);

					IsInitComponent = true;

				}

			}

			else if (IsInitComponent) {

				ResetComponent(false);

				IsInitComponent = false;

			}

		}



		template<typename UICLASS, typename INFOCLASS>

		EditorManagerUI<UICLASS, INFOCLASS>::EditorManagerUI() {

			ZeroMemory(newName, 256);

			Init();

		}

		template<typename UICLASS, typename INFOCLASS>

		EditorManagerUI<UICLASS, INFOCLASS>::~EditorManagerUI() {

			Clear();

		}



		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Init()  {

			SetLabel("UIManager##" + PtrToString((this)));

			SetShowType(UIShowType::TreeNodeEx_Del);

		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Show() {

			auto delIter = mAllUI.end();

			for (auto uiIter = mAllUI.begin(); uiIter != mAllUI.end(); ++uiIter) {

				if (uiIter->second->ShowGui()) {

					delIter = uiIter;

				}

			}

			if (delIter != mAllUI.end()) {

				Remove(delIter->first);

			}

			if (ImGui::InputText("##newName", newName, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {

				IsAddObject = true;

			}

			ImGui::SameLine();

			if (ImGui::Button("Add")) {

				IsAddObject = true;

			}

			if (IsAddObject) {

				Name name(newName);

				if (name == "") {

					PromptDialog::_PromptDialog_ClickType res = CreateObjectNameNULL.Show();

					if (res != PromptDialog::_PromptDialog_ClickType::NONE) {

						IsAddObject = false;

					}

				}

				else if (IsExist(name)) {

					PromptDialog::_PromptDialog_ClickType res = CreateObjectNameExist.Show();

					if (res != PromptDialog::_PromptDialog_ClickType::NONE) {

						IsAddObject = false;

					}

				}

				else {

					Add(name, New(name));

					IsAddObject = false;

				}

			}

		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Update() { }

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Load() { }

		template<typename UICLASS, typename INFOCLASS>

		UICLASS* EditorManagerUI<UICLASS, INFOCLASS>::New(const Name& name) {

			UICLASS* pUI = new UICLASS();

			//IKComponent* pData = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->CreateIKComponent(name, nInfoType);

			//pUI->SetIKComponent(pData);

			pUI->SetLabel(name);

			pUI->DefaultInitialize();

			return pUI;

		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Clear() {

			for (auto pairUI : mAllUI) {

				_ECHO_DELETE(pairUI.second);

				EditorFinalIKUI::instance()->GetCurUI()->RemoveNameFromPool(pairUI.first);

			}

			mAllUI.clear();

		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Add(const Name& name, UICLASS* pui) {

			if (pui) {

				auto pairUI = mAllUI.find(name);

				if (pairUI == mAllUI.end()) {

					EditorFinalIKUI::instance()->GetCurUI()->AddNameToPool(name);

				}

				else {

					_ECHO_DELETE(pairUI->second);

				}

				mAllUI[name] = pui;

			}

		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::Remove(const Name& name) {

			auto delIter = mAllUI.find(name);

			if (delIter != mAllUI.end()) {

				EditorFinalIKUI::instance()->GetCurUI()->RemoveNameFromPool(name);

				_ECHO_DELETE(delIter->second);

				mAllUI.erase(delIter);

			}

		}

		template<typename UICLASS, typename INFOCLASS>

		bool EditorManagerUI<UICLASS, INFOCLASS>::IsExist(Name& name) {

			return EditorFinalIKUI::instance()->GetCurUI()->NameIsExist(name);

		}

		template<typename UICLASS, typename INFOCLASS>

		UICLASS* EditorManagerUI<UICLASS, INFOCLASS>::Find(Name& name) {

			auto p = mAllUI.find(name);

			if (p != mAllUI.end()) {

				return p->second;

			}

			return nullptr;

		}

		template<typename UICLASS, typename INFOCLASS>

		std::map<Name, UICLASS*>* EditorManagerUI<UICLASS, INFOCLASS>::GetData() {

			return &mAllUI;

		}

		template<typename UICLASS, typename INFOCLASS>

		bool EditorManagerUI<UICLASS, INFOCLASS>::Empty() {

			return mAllUI.empty();

		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::ReadInfo(FinalIKInfo* pFinalIKInfo) {



			for (auto pairInfo : pFinalIKInfo->mAllIKMap) {

				if (pairInfo.second->GetType() != mInfoType) {

					continue;

				}

				UICLASS* pui = new UICLASS();

				INFOCLASS* pInfo = (INFOCLASS*)pairInfo.second;







				pui->SetLabel(pInfo->GetName());

				pui->ReadInfo(pInfo);

				Add(pairInfo.first, pui);

			}



		}

		template<typename UICLASS, typename INFOCLASS>

		void EditorManagerUI<UICLASS, INFOCLASS>::WriteInfo(FinalIKInfo* pFinalIKInfo) {

			for (auto pairUI : mAllUI) {

				INFOCLASS* info = (INFOCLASS*)pFinalIKInfo->CreateIKInfo(pairUI.first, mInfoType);

				if (info) {

					pairUI.second->WriteInfo(info);

					info->SetName(Name(pairUI.second->GetLabel()));

				}

			}

		}





		EditorSelectUI::EditorSelectUI() : mSelectName("None##" + PtrToString(this)) { Init();}

		EditorSelectUI::~EditorSelectUI() { /*此处不对m_pNode进行处理，派生类应该判断m_pNode是否删除*/ }

		void EditorSelectUI::Init() {}

		void EditorSelectUI::Update() {}

		void EditorSelectUI::Load() {}

		void EditorSelectUI::MessageResponse() {}

		void EditorSelectUI::SetSelectName(const char* name) {

			mSelectName = Name(std::string(name) + "##" + PtrToString(this));

		}

		const ImVec4& EditorSelectUI::ButtonColor() {

			return DefaultButtonColor;

		}

		void EditorSelectUI::Show()

		{

			bool IsDisable = mIsJudge?CURRENTALLUI->GetIsDisable():false;

			if (!IsDisable) {

				ImGui::PushStyleColor(ImGuiCol_Button, ButtonColor());

			}

			else {

				ImGui::PushStyleColor(ImGuiCol_Button, DisableStateColor);

			}

			bool IsClick1 = ImGui::Button(mSelectName.c_str(), ImVec2(std::max(ImGui::GetCurrentWindow()->DC.ItemWidth - 57.0f, 10.0f), 0.f));

			if (!IsDisable) MessageResponse();

			ImGui::SameLine();

			bool IsClick2 = ImGui::Button((std::string("Sel##SelectButton") + GetLabel()).c_str(), ImVec2(50.0f, 0.f));

			ImGui::SameLine();

			ImGui::Text(GetLabel());

			ImGui::PopStyleColor(1);

			OnButton1(IsClick1);

			OnButton2(!IsDisable && IsClick2);



		}





		template<typename T>

		bool EditorSelectUI::OnButton2UseMap(bool IsClick, const std::map<Name, T*>* mp, std::string& PopupLabel, T** selectItem) {



			if (IsClick) {

				ImGui::OpenPopup(PopupLabel.c_str());

				ZeroMemory(filterName, 256);

			}

			ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() * 0.5f));

			if (ImGui::BeginPopup(PopupLabel.c_str())) {



				ImGui::SetNextItemWidth(ImGui::GetWindowWidth());

				ImGui::InputText("##filterName", filterName, 256, ImGuiInputTextFlags_EnterReturnsTrue);

				std::string fName(filterName);

				std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

				bool IsFilter = fName != "";

				if (ImGui::Selectable("None")) {

					*selectItem = nullptr;

				}

				if (mp == nullptr) {

					ImGui::EndPopup();

					return true;

				}

				for (auto iter = mp->begin(); iter != mp->end(); ++iter)

				{

					std::string str(iter->first.c_str());

					std::transform(str.begin(), str.end(), str.begin(), ::tolower);

					if (!IsFilter || str.find(fName) != -1) {

						if (ImGui::Selectable(iter->first.c_str())) {

							*selectItem = iter->second;

						}

					}

				}

				ImGui::EndPopup();

				return true;

			}

			return false;

		}





		bool EditorSelectUI::OnButton2UseVector(bool IsClick, const std::vector<Name>& names, std::string& PopupLabel, Name& selectItem) {



			if (IsClick) {

				ImGui::OpenPopup(PopupLabel.c_str());

				ZeroMemory(filterName, 256);

			}

			ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() * 0.5f));

			if (ImGui::BeginPopup(PopupLabel.c_str())) {



				ImGui::SetNextItemWidth(ImGui::GetWindowWidth());

				ImGui::InputText("##filterName", filterName, 256, ImGuiInputTextFlags_EnterReturnsTrue);

				std::string fName(filterName);

				std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

				bool IsFilter = fName != "";

				if (ImGui::Selectable("None")) {

					selectItem = NullName;

				}

				if (names.empty()) {

					ImGui::EndPopup();

					return true;

				}

				for (auto iter = names.begin(); iter != names.end(); ++iter)

				{

					std::string str(iter->c_str());

					std::transform(str.begin(), str.end(), str.begin(), ::tolower);

					if (!IsFilter || str.find(fName) != -1) {

						if (ImGui::Selectable(iter->c_str())) {

							selectItem = *iter;

						}

					}

				}

				ImGui::EndPopup();

				return true;

			}

			return false;



		}



		void EditorSelectUI::SetIsJudge(bool IsJudge) {

			mIsJudge = IsJudge;

		}

	};



	//ALLInfoRW

	namespace EditorFinalIK {



		void EFIKLimbIKUI::ReadInfo(LimbIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			FixTransforms=			pInfo->fixTransforms ;

			PositionWeight=			pInfo->IKPositionWeight ;

			RotationWeight=			pInfo->IKRotationWeight ;

			BendNormal=			pInfo->bendNormal;

			AvatarIKGoal=			pInfo->goal ;

			MaintainRotation=			pInfo->maintainRotationWeight ;

			BendModifier=			pInfo->bendModifier ;

			BendModifierWeight=			pInfo->bendModifierWeight ;



			if (pInfo->m_vtBoneName.size() == 3) {

				Bone1.SetNode(pInfo->m_vtBoneName[0]);

				Bone2.SetNode(pInfo->m_vtBoneName[1]);

				Bone3.SetNode(pInfo->m_vtBoneName[2]);

			}

			//Target.SetNode(pInfo->targetName);

		}

		void EFIKLimbIKUI::WriteInfo(LimbIKInfo* pInfo) {



			if (!pInfo) {

				return;

			}

			pInfo->fixTransforms = FixTransforms;

			pInfo->IKPositionWeight = PositionWeight;

			pInfo->IKRotationWeight = RotationWeight;

			pInfo->bendNormal = BendNormal;

			pInfo->goal = AvatarIKGoal;

			pInfo->maintainRotationWeight = MaintainRotation;

			pInfo->bendModifier = BendModifier;

			pInfo->bendModifierWeight = BendModifierWeight;



			pInfo->m_vtBoneName.clear();

			WorldNode* pNode = Bone1.GetNode();

			if (pNode) {

				pInfo->m_vtBoneName.push_back(pNode->getName());

			}

			pNode = Bone2.GetNode();

			if (pNode) {

				pInfo->m_vtBoneName.push_back(pNode->getName());

			}

			pNode = Bone3.GetNode();

			if (pNode) {

				pInfo->m_vtBoneName.push_back(pNode->getName());

			}

			/*pNode = Target.GetNode();

			if (pNode) {

				pInfo->targetName = pNode->getName();

			}*/

		}



		void EFIKFullBodyBipedIKUI::ReadInfo(FullBodyBipedIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			FixTransforms = pInfo->fixTransforms;

			References.ReadInfo(&pInfo->references);



			Weight = pInfo->weight;

			Iterations = pInfo->iterations;



			Solver.ReadInfo(&pInfo->solver);

		}

		void EFIKFullBodyBipedIKUI::WriteInfo(FullBodyBipedIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->fixTransforms = FixTransforms;

			References.WriteInfo(&pInfo->references);



			pInfo->weight = Weight;

			pInfo->iterations = Iterations;



			Solver.WriteInfo(&pInfo->solver);

		}



		void EFIKGroundingUI::ReadInfo(GroundingInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			Layers.SetData(pInfo->layers);

			MaxStep = pInfo->maxStep;

			HeightOffset = pInfo->heightOffset;

			FootSpeed = pInfo->footSpeed;

			FootRadius = pInfo->footRadius;

			Prediction = pInfo->prediction;

			FootRotationWeight = pInfo->footRotationWeight;

			FootRotationSpeed = pInfo->footRotationSpeed;

			MaxFootRotationAngle = pInfo->maxFootRotationAngle;

			RotateSolver = pInfo->rotateSolver;

			PelvisSpeed = pInfo->pelvisSpeed;

			PelvisDamper = pInfo->pelvisDamper;

			LowerPelvisWeight = pInfo->lowerPelvisWeight;

			LiftPelvisWeight = pInfo->liftPelvisWeight;

			RootSphereCastRadius = pInfo->rootSphereCastRadius;

			OverstepFallsDown = pInfo->overstepFallsDown;

			Quality = pInfo->quality;

		}

		void EFIKGroundingUI::WriteInfo(GroundingInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->layers = Layers.GetData();

			pInfo->maxStep = MaxStep;

			pInfo->heightOffset = HeightOffset;

			pInfo->footSpeed = FootSpeed;

			pInfo->footRadius = FootRadius;

			pInfo->prediction = Prediction;

			pInfo->footRotationWeight = FootRotationWeight;

			pInfo->footRotationSpeed = FootRotationSpeed;

			pInfo->maxFootRotationAngle = MaxFootRotationAngle;

			pInfo->rotateSolver = RotateSolver;

			pInfo->pelvisSpeed = PelvisSpeed;

			pInfo->pelvisDamper = PelvisDamper;

			pInfo->lowerPelvisWeight = LowerPelvisWeight;

			pInfo->liftPelvisWeight = LiftPelvisWeight;

			pInfo->rootSphereCastRadius = RootSphereCastRadius;

			pInfo->overstepFallsDown = OverstepFallsDown;

			pInfo->quality = Quality;

		}



		void EFIKBipedReferencesUI::ReadInfo(BipedReferencesInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			Pelvis.SetNode(pInfo->pelvis);

			LeftThigh.SetNode(pInfo->leftThigh);

			LeftCalf.SetNode(pInfo->leftCalf);

			LeftFoot.SetNode(pInfo->leftFoot);

			RightThigh.SetNode(pInfo->rightThigh);

			RightCalf.SetNode(pInfo->rightCalf);

			RightFoot.SetNode(pInfo->rightFoot);

			LeftUpperArm.SetNode(pInfo->leftUpperArm);

			LeftForearm.SetNode(pInfo->leftForearm);

			LeftHand.SetNode(pInfo->leftHand);

			RightUpperArm.SetNode(pInfo->rightUpperArm);

			RightForearm.SetNode(pInfo->rightForearm);

			RightHand.SetNode(pInfo->rightHand);

			Head.SetNode(pInfo->head);



			Spine.Resize((int)pInfo->spine.size());

			for (int i = 0; i != pInfo->spine.size(); ++i) {

				Spine.Get(i)->SetNode(pInfo->spine[i]);

			}



			Eyes.Resize((int)pInfo->eyes.size());

			for (int i = 0; i != pInfo->eyes.size(); ++i) {

				Eyes.Get(i)->SetNode(pInfo->eyes[i]);

			}

		}

		void EFIKBipedReferencesUI::WriteInfo(BipedReferencesInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			 pInfo->pelvis = Pelvis.GetNodeName(); 

			pInfo->leftThigh  = LeftThigh.GetNodeName();

			pInfo->leftCalf  = LeftCalf.GetNodeName();

			pInfo->leftFoot  = LeftFoot.GetNodeName();

			pInfo->rightThigh  = RightThigh.GetNodeName();

			pInfo->rightCalf  = RightCalf.GetNodeName();

			pInfo->rightFoot  = RightFoot.GetNodeName();

			pInfo->leftUpperArm  = LeftUpperArm.GetNodeName();

			pInfo->leftForearm  = LeftForearm.GetNodeName();

			pInfo->leftHand  = LeftHand.GetNodeName();

			pInfo->rightUpperArm  = RightUpperArm.GetNodeName();

			pInfo->rightForearm  = RightForearm.GetNodeName();

			pInfo->rightHand  = RightHand.GetNodeName();

			pInfo->head  = Head.GetNodeName();



			pInfo->spine.clear();

			for (int i = 0; i != Spine.size(); ++i) {

				WorldNode* pNode = Spine.At(i);

				if (pNode) {

					pInfo->spine.push_back(pNode->getName());

				}

			}





			pInfo->eyes.resize(Eyes.size());

			for (int i = 0; i != Eyes.size(); ++i) {

				WorldNode* pNode = Eyes.At(i);

				if (pNode) {

					pInfo->eyes.push_back(pNode->getName());

				}

			}



		}



		void EFIKEffectorUI::ReadInfo(IKEffectorInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			PositionWeight = pInfo->positionWeight;

			EffectChildNodes = pInfo->effectChildNodes;

			RotationWeight = pInfo->rotationWeight;

			MaintainRelativePositionWeight = pInfo->maintainRelativePositionWeight;

			Locator.SetData(&pInfo->locatorName);

		}

		void EFIKEffectorUI::WriteInfo(IKEffectorInfo* pInfo) {

			if (!pInfo) {

				return;

			}



			pInfo->positionWeight = PositionWeight;

			pInfo->effectChildNodes = EffectChildNodes;

			pInfo->rotationWeight = RotationWeight;

			pInfo->maintainRelativePositionWeight = MaintainRelativePositionWeight;

			pInfo->locatorName = Locator.GetDataName();

		}



		void EFIKFBIKChainUI::ReadInfo(FBIKChainInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			Pull = pInfo->pull;

			Reach = pInfo->reach;

			Push = pInfo->push;

			PushParent = pInfo->pushParent;



			ReachSmoothing = pInfo->reachSmoothing;

			PushSmoothing = pInfo->pushSmoothing;



			//BendGoal.SetNode(pInfo->bendGoal);

			BendGoalWeight = pInfo->weight;

		}

		void EFIKFBIKChainUI::WriteInfo(FBIKChainInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->pull = Pull;

			pInfo->reach = Reach;

			pInfo->push = Push;

			pInfo->pushParent = PushParent;



			pInfo->reachSmoothing = ReachSmoothing;

			pInfo->pushSmoothing = PushSmoothing;



			//pInfo->bendGoal = BendGoal.GetNodeName();

			pInfo->weight = BendGoalWeight;

		}



		void EFIKMappingLimbUI::ReadInfo(IKMappingLimbInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			MappingWeight = pInfo->weight;

			MaintainRotationWeight = pInfo->maintainRotationWeight;

		}

		void EFIKMappingLimbUI::WriteInfo(IKMappingLimbInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->weight = MappingWeight;

			pInfo->maintainRotationWeight = MaintainRotationWeight;

		}



		void EFIKMappingSpineBoneUI::ReadInfo(IKMappingSpineBoneInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			

			SpineIterations = pInfo->spineIterations;

			SpineTwistWeight = pInfo->spineTwistWeight;

			MaintainRotationWeight = pInfo->maintainRotationWeight;

			

		}

		void EFIKMappingSpineBoneUI::WriteInfo(IKMappingSpineBoneInfo* pInfo) {

			if (!pInfo) {

				return;

			}



			pInfo->spineIterations = SpineIterations;

			pInfo->spineTwistWeight = SpineTwistWeight;

			pInfo->maintainRotationWeight = MaintainRotationWeight;

		}



		void EFIKSolverFullBodyBipedUI::ReadInfo(IKSolverFullBodyBipedInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			BodyEffector.ReadInfo(&pInfo->bodyEffector);

			LeftHandEffector.ReadInfo(&pInfo->leftHandEffector);

			LeftShoulderEffector.ReadInfo(&pInfo->leftShoulderEffector);

			RightHandEffector.ReadInfo(&pInfo->rightHandEffector);

			RightShoulderEffector.ReadInfo(&pInfo->rightShoulderEffector);

			LeftFootEffector.ReadInfo(&pInfo->leftFootEffector);

			LeftThighEffector.ReadInfo(&pInfo->leftThighEffector);

			RightFootEffector.ReadInfo(&pInfo->rightFootEffector);

			RightThighEffector.ReadInfo(&pInfo->rightThighEffector);



			SpineStiffness = pInfo->SpineStiffness;

			PullBodyVertical = pInfo->PullBodyVertical;

			PullBodyHorizontal = pInfo->PullBodyHorizontal;



			LeftArmChain.ReadInfo(&pInfo->leftArmChain);

			RightArmChain.ReadInfo(&pInfo->rightArmChain);

			LeftLegChain.ReadInfo(&pInfo->leftLegChain);

			RightLegChain.ReadInfo(&pInfo->rightLegChain);



			BodyMapping.ReadInfo(&pInfo->bodyMapping);

			LeftArmMapping.ReadInfo(&pInfo->leftArmMapping);

			RightArmMapping.ReadInfo(&pInfo->rightArmMapping);

			LeftLegMapping.ReadInfo(&pInfo->leftLegMapping);

			RightLegMapping.ReadInfo(&pInfo->rightLegMapping);



		}

		void EFIKSolverFullBodyBipedUI::WriteInfo(IKSolverFullBodyBipedInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			BodyEffector.WriteInfo(&pInfo->bodyEffector);

			LeftHandEffector.WriteInfo(&pInfo->leftHandEffector);

			LeftShoulderEffector.WriteInfo(&pInfo->leftShoulderEffector);

			RightHandEffector.WriteInfo(&pInfo->rightHandEffector);

			RightShoulderEffector.WriteInfo(&pInfo->rightShoulderEffector);

			LeftFootEffector.WriteInfo(&pInfo->leftFootEffector);

			LeftThighEffector.WriteInfo(&pInfo->leftThighEffector);

			RightFootEffector.WriteInfo(&pInfo->rightFootEffector);

			RightThighEffector.WriteInfo(&pInfo->rightThighEffector);



			pInfo->SpineStiffness = SpineStiffness;

			pInfo->PullBodyVertical = PullBodyVertical;

			pInfo->PullBodyHorizontal = PullBodyHorizontal;



			LeftArmChain.WriteInfo(&pInfo->leftArmChain);

			RightArmChain.WriteInfo(&pInfo->rightArmChain);

			LeftLegChain.WriteInfo(&pInfo->leftLegChain);

			RightLegChain.WriteInfo(&pInfo->rightLegChain);



			BodyMapping.WriteInfo(&pInfo->bodyMapping);

			LeftArmMapping.WriteInfo(&pInfo->leftArmMapping);

			RightArmMapping.WriteInfo(&pInfo->rightArmMapping);

			LeftLegMapping.WriteInfo(&pInfo->leftLegMapping);

			RightLegMapping.WriteInfo(&pInfo->rightLegMapping);



		}



		void EFIKGrounderQuadrupedUI::ReadInfo(GrounderQuadrupedInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			Weight = pInfo->weight;

			RootRotationWeight = pInfo->rootRotationWeight;

			MinRootRotation = pInfo->minRootRotation;

			MaxRootRotation = pInfo->maxRootRotation;

			RootRotationSpeed = pInfo->rootRotationSpeed;

			MaxLegOffset = pInfo->maxLegOffset;

			MaxForeLegOffset = pInfo->maxForeLegOffset;

			MaintainHeadRotationWeight = pInfo->maintainHeadRotationWeight;



			mPelvis.SetNode(pInfo->pelvis);

			mLastSpineBone.SetNode(pInfo->lastSpineBone);

			mHead.SetNode(pInfo->head);



			mSolver.ReadInfo(&pInfo->solver);

			mForelegSolver.ReadInfo(&pInfo->forelegSolver);

			

			//这块不能再limbMananger之前执行

			auto* pLimbMap = EditorFinalIKUI::instance()->GetCurUI()->GetLimbMap();

			mLegs.Clear();

			for (int i = 0; i != pInfo->legNames.size(); ++i) {

				auto limb = pLimbMap->find(pInfo->legNames[i]);

				if (limb != pLimbMap->end()) {

					mLegs.push_back(limb->second);

				}

			}



			mForelegs.Clear();

			for (int i = 0; i != pInfo->forelegNames.size(); ++i) {

				auto limb = pLimbMap->find(pInfo->forelegNames[i]);

				if (limb != pLimbMap->end()) {

					mForelegs.push_back(limb->second);

				}

			}

			if (m_pData) {

				m_pData->SetIKInfoName(pInfo->GetName());

			}

		}

		void EFIKGrounderQuadrupedUI::WriteInfo(GrounderQuadrupedInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->weight = Weight;

			pInfo->rootRotationWeight = RootRotationWeight;

			pInfo->minRootRotation = MinRootRotation;

			pInfo->maxRootRotation = MaxRootRotation;

			pInfo->rootRotationSpeed = RootRotationSpeed;

			pInfo->maxLegOffset = MaxLegOffset;

			pInfo->maxForeLegOffset = MaxForeLegOffset;

			pInfo->maintainHeadRotationWeight = MaintainHeadRotationWeight;



			pInfo->pelvis = mPelvis.GetNodeName();

			pInfo->lastSpineBone = mLastSpineBone.GetNodeName();

			pInfo->head = mHead.GetNodeName();



			mSolver.WriteInfo(&pInfo->solver);

			mForelegSolver.WriteInfo(&pInfo->forelegSolver);



			pInfo->legNames.clear();

			for (int i = 0; i != mLegs.size(); ++i) {

				/*EchoIK::LimbIK* pIK = mLegs.At(i)->GetData();

				if (!pIK) {

					continue;

				}*/

				//Name name(pIK->GetIKInfoName());

				Name name(mLegs.Get(i)->GetData()->GetLabel());

				pInfo->legNames.push_back(name);

			}



			pInfo->forelegNames.clear();

			for (int i = 0; i != mForelegs.size(); ++i) {

				/*EchoIK::LimbIK* pIK = mForelegs.At(i)->GetData();

				if (!pIK) {

					continue;

				}

				Name name(pIK->GetIKInfoName());*/

				Name name(mForelegs.Get(i)->GetData()->GetLabel());

				pInfo->forelegNames.push_back(name);

			}

		}



		void EFIKGrounderIKUI::ReadInfo(GrounderIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			Weight = pInfo->weight;

			RootRotationWeight = pInfo->rootRotationWeight;

			RootRotationSpeed = pInfo->rootRotationSpeed;

			MaxRootRotationAngle = pInfo->maxRootRotationAngle;



			mPelvis.SetNode(pInfo->pelvis);

			

			mSolver.ReadInfo(&pInfo->solver);



			auto* pLimbMap = EditorFinalIKUI::instance()->GetCurUI()->GetLimbMap();

			mLegs.Clear();

			for (int i = 0; i != pInfo->legNames.size(); ++i) {

				auto limb = pLimbMap->find(pInfo->legNames[i]);

				if (limb != pLimbMap->end()) {

					mLegs.push_back(limb->second);

				}

			}

			if (m_pData) {

				m_pData->SetIKInfoName(pInfo->GetName());

			}

		}

		void EFIKGrounderIKUI::WriteInfo(GrounderIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->weight = Weight;

			pInfo->rootRotationWeight = RootRotationWeight;

			pInfo->rootRotationSpeed = RootRotationSpeed;

			pInfo->maxRootRotationAngle = MaxRootRotationAngle;



			pInfo->pelvis = mPelvis.GetNodeName();

			mSolver.WriteInfo(&pInfo->solver);



			pInfo->legNames.clear();

			for (int i = 0; i != mLegs.size(); ++i) {

				/*EchoIK::LimbIK* pIK = mLegs.At(i)->GetData();

				if (!pIK) {

					continue;

				}

				Name name(pIK->GetIKInfoName());*/

				Name name(mLegs.Get(i)->GetData()->GetLabel());

				pInfo->legNames.push_back(name);

			}

		}





		void EFIKSpineEffectorUI::ReadInfo(SpineEffectorInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			effectorType = pInfo->effectorType;

			horizontalWeight = pInfo->horizontalWeight;

			verticalWeight = pInfo->verticalWeight;

		}

		void EFIKSpineEffectorUI::WriteInfo(SpineEffectorInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->effectorType = effectorType;

			pInfo->horizontalWeight = horizontalWeight;

			pInfo->verticalWeight = verticalWeight;

		}





		void EFIKGrounderFBBIKUI::ReadInfo(GrounderFBBIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}



			Weight = pInfo->weight;

			SpineBend = pInfo->spineBend;

			SpineSpeed = pInfo->spineSpeed;

			mSolver.ReadInfo(&pInfo->solver);



			size_t size = pInfo->spine.size();

			mSpineList.Resize((int)size);

			for (int i = 0; i != size; ++i) {

				mSpineList.Get(i)->ReadInfo(&pInfo->spine[i]);

			}

			auto pFBBIIKManager = EditorFinalIKUI::instance()->GetCurUI()->GetFBBIKManager();

			ik.SetData(pFBBIIKManager->Find(pInfo->ik));

			if (m_pData) {

				m_pData->SetIKInfoName(pInfo->GetName());

			}

		}

		void EFIKGrounderFBBIKUI::WriteInfo(GrounderFBBIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}



			pInfo->weight = Weight;

			pInfo->spineBend = SpineBend;

			pInfo->spineSpeed = SpineSpeed;

			mSolver.WriteInfo(&pInfo->solver);



			size_t size = mSpineList.size();

			pInfo->spine.resize(size);

			for (int i = 0; i != size; ++i) {

				mSpineList.Get(i)->WriteInfo(&pInfo->spine[i]);

			}

			pInfo->ik = ik.GetFBBIKName();

		}



// 		void EFIKTargetNodeUI::ReadInfo(TargetNodeInfo* pInfo) {

// 			TargetPositionOffset = pInfo->PositionOffset;

// 			TargetRotationOffset = pInfo->RotationOffset;

// 		}

// 		void EFIKTargetNodeUI::WriteInfo(TargetNodeInfo* pInfo) {

// 			pInfo->PositionOffset = TargetPositionOffset;

// 			pInfo->RotationOffset = TargetRotationOffset;

// 		}



		void EFIKTargetFBBIKUI::ReadInfo(TargetFBBIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			auto pFBBIIKManager = EditorFinalIKUI::instance()->GetCurUI()->GetFBBIKManager();

			ik.SetData(pFBBIIKManager->Find(pInfo->ik));



// 			Body.ReadInfo(&pInfo->body);

// 			LeftHand.ReadInfo(&pInfo->leftHand);

// 			LeftShoulder.ReadInfo(&pInfo->leftShoulder);

// 			RightHand.ReadInfo(&pInfo->rightHand);

// 			RightShoulder.ReadInfo(&pInfo->rightShoulder);

// 			LeftFoot.ReadInfo(&pInfo->leftFoot);

// 			LeftThigh.ReadInfo(&pInfo->leftThigh);

// 			RightFoot.ReadInfo(&pInfo->rightFoot);

// 			RightThigh.ReadInfo(&pInfo->rightThigh);





		}

		void EFIKTargetFBBIKUI::WriteInfo(TargetFBBIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			pInfo->ik = ik.GetFBBIKName();



// 			Body.WriteInfo(&pInfo->body);

// 			LeftHand.WriteInfo(&pInfo->leftHand);

// 			LeftShoulder.WriteInfo(&pInfo->leftShoulder);

// 			RightHand.WriteInfo(&pInfo->rightHand);

// 			RightShoulder.WriteInfo(&pInfo->rightShoulder);

// 			LeftFoot.WriteInfo(&pInfo->leftFoot);

// 			LeftThigh.WriteInfo(&pInfo->leftThigh);

// 			RightFoot.WriteInfo(&pInfo->rightFoot);

// 			RightThigh.WriteInfo(&pInfo->rightThigh);

		}



		void EFIKAimIKUI::ReadInfo(AimIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			fixTransforms = pInfo->fixTransforms;

			/*aimTransform.SetNode(pInfo->aimTransform);

			target.SetNode(pInfo->target);*/

			aimTransformWorldNode.SetData(nullptr);

			targetWorldNode.SetData(nullptr);

			poleTarget.SetNode(nullptr);

			axis = pInfo->axis;

			poleAxis = pInfo->poleAxis;

			weight = pInfo->weight;

			poleWeight = pInfo->poleWeight;

			tolerance = pInfo->tolerance;

			maxIterations = pInfo->maxIterations;

			clampWeight = pInfo->clampWeight;

			clampSmoothing = pInfo->clampSmoothing;

			useRotationLimits = pInfo->useRotationLimits;

			XY = pInfo->XY;



			bones.Resize((int)pInfo->bones.size());

			for (int i = 0; i != pInfo->bones.size(); ++i) {

				bones.Get(i)->ReadInfo(&pInfo->bones[i]);

			}

		}

		void EFIKAimIKUI::WriteInfo(AimIKInfo* pInfo) {

			if (!pInfo) {

				return;

			}

			

			pInfo->fixTransforms = fixTransforms;

			/*pInfo->aimTransform = aimTransform.GetNodeName();

			pInfo->target = target.GetNodeName();*/

			/*pInfo->aimTransform = aimTransformWorldNode.GetDataName();

			pInfo->target = targetWorldNode.GetDataName();

			pInfo->poleTarget = poleTarget.GetNodeName();*/

			pInfo->axis = axis;

			pInfo->poleAxis = poleAxis;

			pInfo->weight = weight;

			pInfo->poleWeight = poleWeight;

			pInfo->tolerance = tolerance;

			pInfo->maxIterations = maxIterations;

			pInfo->clampWeight = clampWeight;

			pInfo->clampSmoothing = clampSmoothing;

			pInfo->useRotationLimits = useRotationLimits;

			pInfo->XY = XY;



			pInfo->bones.clear();

			int size = bones.size();

			pInfo->bones.resize((size_t)size);

			for (int i = 0; i != bones.size(); ++i) {

				bones.Get(i)->WriteInfo(&pInfo->bones[i]);

			}

		}



		void EFIKFBBIKHeadEffectorUI::ReadInfo(FBBIKHeadEffectorInfo* pInfo) {

			

			if (!pInfo) {

				return;

			}

			//在EFIKFullBodyBipedIKUI之后执行

			auto pFBBIIKManager = EditorFinalIKUI::instance()->GetCurUI()->GetFBBIKManager();

			ik.SetData(pFBBIIKManager->Find(pInfo->ik));



			positionWeight = pInfo->positionWeight;

			bodyWeight = pInfo->bodyWeight;

			thighWeight = pInfo->thighWeight;

			handsPullBody = pInfo->handsPullBody;

			rotationWeight = pInfo->rotationWeight;

			bodyClampWeight = pInfo->bodyClampWeight;

			headClampWeight = pInfo->headClampWeight;

			bendWeight = pInfo->bendWeight;

			CCDWeight = pInfo->CCDWeight;

			roll = pInfo->roll;

			damper = pInfo->damper;

			postStretchWeight = pInfo->postStretchWeight;

			maxStretch = pInfo->maxStretch;

			stretchDamper = pInfo->stretchDamper;

			fixHead = pInfo->fixHead;

			chestDirection = pInfo->chestDirection;

			chestDirectionWeight = pInfo->chestDirectionWeight;



			bendBones.Clear();

			int size = (int)pInfo->bendBones.size();

			bendBones.Resize(size);

			for (int i = 0; i != size; ++i) {

				bendBones.Get(i)->ReadInfo(&pInfo->bendBones[i]);

			}

			//CCDBones

			CCDBones.Resize((int)pInfo->CCDBones.size());

			for (int i = 0; i != pInfo->CCDBones.size(); ++i) {

				CCDBones.Get(i)->SetNode(pInfo->CCDBones[i]);

			}

			//stretchBones

			stretchBones.Resize((int)pInfo->stretchBones.size());

			for (int i = 0; i != pInfo->stretchBones.size(); ++i) {

				stretchBones.Get(i)->SetNode(pInfo->stretchBones[i]);

			}

			//chestBones

			chestBones.Resize((int)pInfo->chestBones.size());

			for (int i = 0; i != pInfo->chestBones.size(); ++i) {

				chestBones.Get(i)->SetNode(pInfo->chestBones[i]);

			}

		

		}

		void EFIKFBBIKHeadEffectorUI::WriteInfo(FBBIKHeadEffectorInfo* pInfo) {

			

			if (!pInfo) {

				return;

			}

			pInfo->ik = ik.GetFBBIKName();



			pInfo->positionWeight = positionWeight;

			pInfo->bodyWeight = bodyWeight;

			pInfo->thighWeight = thighWeight;

			pInfo->handsPullBody = handsPullBody;

			pInfo->rotationWeight = rotationWeight;

			pInfo->bodyClampWeight = bodyClampWeight;

			pInfo->headClampWeight = headClampWeight;

			pInfo->bendWeight = bendWeight;

			pInfo->CCDWeight = CCDWeight;

			pInfo->roll = roll;

			pInfo->damper = damper;

			pInfo->postStretchWeight = postStretchWeight;

			pInfo->maxStretch = maxStretch;

			pInfo->stretchDamper = stretchDamper;

			pInfo->fixHead = fixHead;

			pInfo->chestDirection = chestDirection;

			pInfo->chestDirectionWeight = chestDirectionWeight;





			pInfo->bendBones.clear();

			int size = bendBones.size();

			pInfo->bendBones.resize((size_t)size);

			for (int i = 0; i != bendBones.size(); ++i) {

				bendBones.Get(i)->WriteInfo(&pInfo->bendBones[i]);

			}

			//CCDBones

			pInfo->CCDBones.resize(CCDBones.size());

			for (int i = 0; i != CCDBones.size(); ++i) {

				pInfo->CCDBones[i] = CCDBones.Get(i)->GetNodeName();

			}

			//stretchBones

			pInfo->stretchBones.resize(stretchBones.size());

			for (int i = 0; i != stretchBones.size(); ++i) {

				pInfo->stretchBones[i] = stretchBones.Get(i)->GetNodeName();

			}

			//chestBones

			pInfo->chestBones.resize(chestBones.size());

			for (int i = 0; i != chestBones.size(); ++i) {

				pInfo->chestBones[i] = chestBones.Get(i)->GetNodeName();

			}

		}





		void EFIKNodeWeightUI::ReadInfo(NodeWeightInfo* pInfo) {

			transform.SetNode(pInfo->transform);

			weight = pInfo->weight;

		}

		void EFIKNodeWeightUI::WriteInfo(NodeWeightInfo* pInfo) {

			pInfo->transform = transform.GetNodeName();

			pInfo->weight = weight;

		}

	};







	//EFIKEffectorList

	namespace EditorFinalIK {



		EFIKSpineEffectorUI::EFIKSpineEffectorUI() { Init(); }

		EFIKSpineEffectorUI::~EFIKSpineEffectorUI() {  }

		void EFIKSpineEffectorUI::Init() {

			SetShowType(UIShowType::TreeNode);

		}

		void EFIKSpineEffectorUI::Show() {

			ImGui::Combo("Avatar IK Goal", &effectorType, EffectorEnumLabel.data(), EffectorTypeEnum::NumEffectors);

			IKDragFloat("Horizontal Weight", &horizontalWeight, 0.01f, 0.f, 1.0f);

			IKDragFloat("Vertical Weight", &verticalWeight, 0.01f, 0.f, 1.0f);



		}

		void EFIKSpineEffectorUI::Update(EchoIK::GrounderFBBIK::SpineEffector* m_pData) {

			if (!m_pData) {

				return;

			}

			m_pData->effectorType = (EffectorTypeEnum)effectorType;

			m_pData->horizontalWeight = horizontalWeight;

			m_pData->verticalWeight = verticalWeight;

		}

		void EFIKSpineEffectorUI::Load(EchoIK::GrounderFBBIK::SpineEffector* m_pData) {

			if (!m_pData) {

				return;

			}

			effectorType = m_pData->effectorType;

			horizontalWeight = m_pData->horizontalWeight;

			verticalWeight = m_pData->verticalWeight;

		}

	};



	//EFIKLimbIKUI

	namespace EditorFinalIK {



		EFIKLimbIKUI::EFIKLimbIKUI()

		{

			Init();

		}

		/*EFIKLimbIKUI::EFIKLimbIKUI(RoleObject* pRoleObject) {

			Init();

			SetLabel("LimbIK##" + pRoleObject->getName().toString());

			m_pData = new EchoIK::LimbIK(pRoleObject->getRoleIKComponent());

			SetData(m_pData);

			EditorFinalIKUI::instance()->GetCurUI()->AddPtrToPool(this);

		}*/



		EFIKLimbIKUI::~EFIKLimbIKUI()

		{

		}



		void EFIKLimbIKUI::Init()

		{

			SetShowType(UIShowType::TreeNodeEx_Del);

			Bone1.SetShowType(UIShowType::None);

			Bone2.SetShowType(UIShowType::None);

			Bone3.SetShowType(UIShowType::None);

			Target.SetShowType(UIShowType::None);



			Bone1.SetLabel("Bone1");

			Bone2.SetLabel("Bone2");

			Bone3.SetLabel("Bone3");

			Target.SetLabel("Target");



			Target.SetIsJudge(false);

		}



		void EFIKLimbIKUI::PureShow()

		{



			ImGui::Checkbox("Fix Transforms", &FixTransforms);

			Bone1.ShowGui();

			Bone2.ShowGui();

			Bone3.ShowGui();

			Target.ShowGui();

			IKSliderFloat("Position Weight", &PositionWeight, 0.f, 1.f);

			IKSliderFloat("Rotation Weight", &RotationWeight, 0.f, 1.f);

			if (!IsUseComponent || !m_pData) {

				ImGui::InputFloat3("Bend Normal", BendNormal.ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

			}

			else {

				Vector3 _bendNoraml = m_pData->GetIKSolver()->bendNormal;

				ImGui::InputFloat3("Bend Normal", _bendNoraml.ptr(),"%.3f", ImGuiInputTextFlags_ReadOnly);

			}

			ImGui::Combo("Avatar IK Goal", &AvatarIKGoal, AvatarIKGoalTypeLabels.data(), (int)AvatarIKGoalTypeLabels.size());

			IKSliderFloat("Maintain Rotation", &MaintainRotation, 0.f, 1.f);

			ImGui::Combo("Bend Modifier", &BendModifier, BendModifierTypeLabels.data(), (int)BendModifierTypeLabels.size());

			IKSliderFloat("Bend Modifier Weight", &BendModifierWeight, 0.f, 1.f);



		}



		void EFIKLimbIKUI::Update()

		{

			if (!m_pData) {

				return;

			}

			m_pData->fixTransforms = FixTransforms;



			if (Target.GetData() == nullptr) {

				m_pData->resetLimbTarget();

			}

			else {

				m_pData->setLimbTarget(Target.GetData(), Name::g_cEmptyName);

			}



			auto pData = m_pData->GetIKSolver();

			

			/*WorldNode* roleNode = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->m_pRoleNode;

			if (Bone1.GetNode() && Bone2.GetNode() && Bone3.GetNode()) {

				pData->SetChain(Bone1.GetNode(), Bone2.GetNode(), Bone3.GetNode(), roleNode);

			}*/

			//pData->target = Target.GetData();



			

			pData->IKPositionWeight = PositionWeight;

			pData->IKRotationWeight = RotationWeight;

			//pData->bendNormal = BendNormal;

			pData->goal = (EchoIK::AvatarIKGoal)AvatarIKGoal;//enum

			pData->maintainRotationWeight = MaintainRotation;

			pData->bendModifier = (EchoIK::IKSolverLimb::BendModifier)BendModifier;//enum

			pData->bendModifierWeight = BendModifierWeight;



		}



		void EFIKLimbIKUI::Load()

		{

			/*if (!m_pData) {

				return;

			}*/

			//FixTransforms = m_pData->fixTransforms;

			//EchoIK::IKSolverLimb* pData = m_pData->GetIKSolver();

			//PositionWeight = pData->IKPositionWeight;

			//RotationWeight = pData->IKRotationWeight;

			//BendNormal = pData->bendNormal;

			//AvatarIKGoal = pData->goal;

			//MaintainRotation = pData->maintainRotationWeight;

			//BendModifier = pData->bendModifier;

			//BendModifierWeight = pData->bendModifierWeight;



			//if (pData->bone1) Bone1.SetNode((Bone*)pData->bone1->transform);

			//if (pData->bone2) Bone2.SetNode((Bone*)pData->bone2->transform);

			//if (pData->bone3) Bone3.SetNode((Bone*)pData->bone3->transform);

			//Target.SetNode((Bone*)pData->target);

		}

		UINT32 EFIKLimbIKUI::JudgeCanUseComponent() {

			if (Bone1.GetNode() && Bone2.GetNode() && Bone3.GetNode()) {

				return UIErrorType_NoError;

			}

			return UIErrorType_ParametersAreNotFullySet;

		}



		void EFIKLimbIKUI::DefaultInitialize() {

			

			//RoleIKComponent* pRoleIKComponent = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent();



			FixTransforms = false;

			Bone1.SetNode(nullptr);

			Bone2.SetNode(nullptr);

			Bone3.SetNode(nullptr);

			Target.SetData(nullptr);

			PositionWeight = 1.0f;

			RotationWeight = 1.0f;

			BendNormal = { -1.0f,0.0f,0.0f };

			AvatarIKGoal = 0;

			MaintainRotation = 0.0f;

			BendModifier = 0;

			BendModifierWeight = 1.0f;



		}



	};



	//EditorGroupUI

	namespace EditorFinalIK {

	

		WorldNode* EditorNodeGroupUI::At(int n) {

			if (n < mCount) {

				return mGroup[n]->GetNode();

			}

			return nullptr;

		}

		void EditorNodeGroupUI::push_back(WorldNode* p) {

			if (p == nullptr) return;

			mGroup.push_back(new NodeSelectUI());

			mGroup[mCount]->SetLabel("Element" + std::to_string(mCount));

			mGroup[mCount]->SetShowType(elementType);

			mGroup[mCount]->Init();

			mGroup[mCount++]->SetNode(p);

		}

		EditorNodeGroupUI::EditorNodeGroupUI() {

			elementType = UIShowType::None;

		}

		EditorNodeGroupUI::~EditorNodeGroupUI() { Clear(); }



		/*void EditorNodeGroupUI::Load() {

			if (!m_pData) {

				return;

			}

			Clear();

			for (int i = 0; i != m_pData->size(); ++i) {

				push_back(m_pData->at(i));

			}

		}

		void EditorNodeGroupUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->resize(mGroup.size());

			for (int i = 0; i != mGroup.size(); ++i) {

				mGroup[i]->Update();

				(*m_pData)[i] = mGroup[i]->GetNode();

			}

		};*/



		//-----------------------------------------------



		EFIKLimbIKUI* EditorLimbGroupUI::At(int n) {

			if (n < mCount) {

				return mGroup[n]->GetData();

			}

			return nullptr;

		}

		void EditorLimbGroupUI::push_back(EFIKLimbIKUI* p) {

			if (p == nullptr) return;

			mGroup.push_back(new LimbSelectUI());

			mGroup[mCount]->SetLabel("Element" + std::to_string(mCount));

			mGroup[mCount]->SetShowType(elementType);

			mGroup[mCount]->Init();

			mGroup[mCount++]->SetData(p);

		}

		EditorLimbGroupUI::EditorLimbGroupUI() {}

		EditorLimbGroupUI::~EditorLimbGroupUI() {}



		/*void EditorLimbGroupUI::Load() {

			if (!m_pData) {

				return;

			}

			Clear();

			auto* pLimbMap = EditorFinalIKUI::instance()->GetCurUI()->GetLimbMap();

			for (int i = 0; i != m_pData->size(); ++i) {

				auto limb = pLimbMap->find(m_pData->at(i)->GetIKInfoName());

				if (limb != pLimbMap->end()) {

					push_back(limb->second);

				}

			}

		}

		void EditorLimbGroupUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->clear();

			for (int i = 0; i != mGroup.size(); ++i) {

				mGroup[i]->Update();

				EFIKLimbIKUI* pui = mGroup[i]->GetData();

				if (pui) {

					m_pData->push_back(pui->GetData());

				}

			}

		};*/

		

		//-----------------------------------------------

	

		EFIKSpineEffectorGroupUI::EFIKSpineEffectorGroupUI() { elementType = UIShowType::TreeNode; }

		EFIKSpineEffectorGroupUI::~EFIKSpineEffectorGroupUI() {  }



		void EFIKSpineEffectorGroupUI::push_back(EFIKSpineEffectorUI* peffect) {

			if (peffect == nullptr) return;

			mGroup.push_back(peffect);

			++mCount;

		}



		/*void EFIKSpineEffectorGroupUI::Load() {

			if (!m_pData) {

				return;

			}

			Resize((int)m_pData->size());

			for (int i = 0; i != mCount; ++i) {

				mGroup[i]->Load(m_pData->at(i));

			}

		}

		void EFIKSpineEffectorGroupUI::Update() {

			if (!m_pData) {

				return;

			}

			int n = (int)m_pData->size();

			if (n > mCount) {

				for (int i = mCount; i != n; ++i) {

					_ECHO_DELETE((*m_pData)[i]);

				}

				m_pData->resize(mCount);

			}

			else if (n < mCount) {

				m_pData->resize(mCount);

				for (int i = n; i != mCount; ++i) {

					(*m_pData)[i] = new EchoIK::GrounderFBBIK::SpineEffector();

				}

			}

			for (int i = 0; i != mCount; ++i) {

				mGroup[i]->Update((*m_pData)[i]);

			}

		};*/

	};





	//WorldNode

	namespace EditorFinalIK {

		NodeSelectUI::NodeSelectUI()

		{

			m_pData = new NodeUI();

			PopupLabel = std::string("SelectNode##") + PtrToString(this);

			SetIsJudge(true);

		}



		NodeSelectUI::~NodeSelectUI()

		{

			_ECHO_DELETE(m_pData);

		}



		void NodeSelectUI:: OnButton2(bool IsClick)

		{

			const std::map<Name, Bone*>* mp = EditorFinalIKUI::instance()->GetCurUI()->GetSkeletonMap();

			Bone* selItem = (Bone*)GetNode();

			if (OnButton2UseMap(IsClick, mp, PopupLabel, &selItem)) {

				SetNode(selItem != nullptr?EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(selItem->getName()):nullptr);

			}

		}



		void NodeSelectUI::MessageResponse()

		{

			if (ImGui::BeginDragDropTarget()) {

				if (const ImGuiPayload* payLoad = ImGui::AcceptDragDropPayload("SkeletonUIBoneName")) {

					Bone* pBone = nullptr;

					char* temp = new char[256];

					ZeroMemory(temp, 256);

					memcpy(temp, payLoad->Data, 256);



					auto pMap = EditorFinalIKUI::instance()->GetCurUI()->GetSkeletonMap();

					auto iter = pMap->find(Name(temp));

					if (iter != pMap->end()) {

						pBone = iter->second;

						//SetNode(EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(pBone->getName()));

						SetNode(pBone != nullptr ? EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(pBone->getName()) : nullptr);

					}





					_ECHO_DELETES(temp);

				}

				ImGui::EndDragDropTarget();

				return;

			}

		}



		void NodeSelectUI::OnButton1(bool IsClick)

		{

			if (IsClick && m_pData) {

				ImGui::OpenPopup((std::string("ShowNode##") + PtrToString(this)).c_str());

			}

			//ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() * 0.5f));

			if (ImGui::BeginPopup((std::string("ShowNode##") + PtrToString(this)).c_str())) {

				m_pData->ShowGui(UIShowType::None, 0.9f);

				ImGui::EndPopup();

			}

		}



		WorldNode* NodeSelectUI::GetNode() {

			if (!m_pData) {

				return nullptr;

			}

			return m_pData->GetData();

		}



		void NodeSelectUI::SetNode(WorldNode* pNode) {

			if (!pNode) {

				SetSelectName("None");

				m_pData->SetData(pNode);

				return;

			}

			m_pData->SetData(pNode);

			if (pNode->getName() == "") {

				SetSelectName("None");

			}

			else {

				SetSelectName(pNode->getName().c_str());

			}

		}

		void NodeSelectUI::SetNode(Name& name) {

			SetNode(EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(name));

		}

		const Name& NodeSelectUI::GetNodeName() {

			WorldNode* node = GetNode();

			if (node) {

				return node->getName();

			}

			else {

				return NullName;

			}

		}

		const ImVec4& NodeSelectUI::ButtonColor() {

			if (m_pData->GetData()) {

				return DefaultButtonColor;

			}

			return NodeSelectButtonColor;

		}

		NodeUI::NodeUI() { SetShowType(UIShowType::None); Init(); }

		NodeUI::~NodeUI() {}

		void NodeUI::SetData(WorldNode* pNode) {

			m_pData = pNode;

			if (pNode) {

				SetLabel(m_pData->getName().toString() + "##" + PtrToString(this));

			}

			else {

				SetLabel("None##" + PtrToString(this));

			}

			Load();

		}



		void NodeUI::Init()

		{

		}



		void NodeUI::Show()

		{

			/*if (m_pData)

			{

				

				ImGui::InputFloat3("Position", (float*)m_pData->getPosition().ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

				ImGui::InputFloat4("Quaternion", (float*)m_pData->getOrientation().ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

				ImGui::InputFloat3("Scale", (float*)m_pData->getScale().ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

			}*/

		}



		void NodeUI::Update() { }



		void NodeUI::Load()

		{



		}





		EFIKNodeWeightUI::EFIKNodeWeightUI() { Init(); }

		EFIKNodeWeightUI::~EFIKNodeWeightUI() {}



		void EFIKNodeWeightUI::Init() {

			transform.SetLabel("Weight");

			transform.SetShowType(UIShowType::None);

		}

		void EFIKNodeWeightUI::Show() {



			transform.ShowGui(UIShowType::Default, 0.25);

			ImGui::SameLine();

			IKSliderFloat("##weight", &weight, 0.0f, 1.0f);

		}

	}



	//LimbSelectUI

	namespace EditorFinalIK {

		LimbSelectUI::LimbSelectUI() {

			PopupLabel = std::string("SelectLimb##") + PtrToString(this);

			SetIsJudge(true);

		}



		LimbSelectUI::~LimbSelectUI() {}



		void LimbSelectUI::OnButton2(bool IsClick)

		{

			const std::map<Name, EFIKLimbIKUI*>* mp = EditorFinalIKUI::instance()->GetCurUI()->GetLimbMap();

			EFIKLimbIKUI* selItem = m_pData;

			if (OnButton2UseMap(IsClick, mp, PopupLabel, &selItem)) {

				SetData(selItem);

			}

		}



		void LimbSelectUI::MessageResponse() {}



		void LimbSelectUI::OnButton1(bool IsClick)

		{

			if (!EditorFinalIKUI::instance()->GetCurUI()->PtrIsExist(m_pData)) {

				SetData(nullptr);

			}

			if (IsClick && m_pData) {

				ImGui::OpenPopup((std::string("ShowNode##") + GetLabel()).c_str());

			}

			//ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth() * 0.5, ImGui::GetWindowHeight() * 0.5));

			if (ImGui::BeginPopup((std::string("ShowNode##") + GetLabel()).c_str())) {

				m_pData->ShowGui(UIShowType::None, 0.9f);

				ImGui::EndPopup();

			}

		}



		void LimbSelectUI::SetData(EFIKLimbIKUI* pData) {

			m_pData = pData;

			if (m_pData) {

				SetSelectName(m_pData->GetLabel());

			}

			else {

				SetSelectName("None");

			}

		}

		const ImVec4& LimbSelectUI::ButtonColor() {

			if (m_pData) {

				return DefaultButtonColor;

			}

			return LimbSelectButtonColor;

		}

	};



	//EFIKGroundingUI

	namespace EditorFinalIK {





		EFIKGroundingLayers::EFIKGroundingLayers() {

			layerSelects = new bool[(int)(layerTypeLabels.size())];

			memset(layerSelects, false, layerTypeLabels.size());

		}

		EFIKGroundingLayers::~EFIKGroundingLayers() {

			memset(layerSelects, false, layerTypeLabels.size());

			_ECHO_DELETES(layerSelects);

		}



		void EFIKGroundingLayers::Init() {

			

		}

		void EFIKGroundingLayers::Show() {

			bool IsUpdateLayers = false;



			if (ImGui::Checkbox(layerTypeLabels[0], &layerSelects[0])) {

				if (layerSelects[0]) {

					for (int i = 1; i != layerTypeLabels.size(); ++i) {

						layerSelects[i] = false;

					}

					value = 0;

				}

				layerSelects[0] = true;



			}

			if (ImGui::Checkbox(layerTypeLabels[1], &layerSelects[1])) {

				if (layerSelects[1]) {

					layerSelects[0] = false;

					for (int i = 2; i != layerTypeLabels.size(); ++i) {

						layerSelects[i] = true;

					}

					value = INT_MAX;

				}

				else {

					layerSelects[0] = true;

					for (int i = 2; i != layerTypeLabels.size(); ++i) {

						layerSelects[i] = false;

					}

					value = 0;

				}

			}

			for (int i = 2; i != layerTypeLabels.size(); ++i) {

				IsUpdateLayers |= ImGui::Checkbox(layerTypeLabels[i], &layerSelects[i]);

			}



			if (IsUpdateLayers) UpdateLayers();

		}

		void EFIKGroundingLayers::Update() {



		}

		void EFIKGroundingLayers::Load() {



		}

		void EFIKGroundingLayers::SetData(unsigned int n) {

			value = n;

			LoadLayers();

		}

		unsigned int EFIKGroundingLayers::GetData() {

			return value;

		}



		void EFIKGroundingLayers::UpdateLayers() {

			value = 0;

			bool t = true;

			for (int i = 2; i != layerTypeLabels.size(); ++i) {

				if (layerSelects[i]) {

					value |= EnumToInt_layer[i];

				}

				else {

					t = false;

				}

			}

			if (value == 0) {

				layerSelects[0] = true;

				layerSelects[1] = false;

			}

			else if (t) {

				layerSelects[0] = false;

				layerSelects[1] = true;

				value = INT_MAX;

			}

			else {

				layerSelects[0] = false;

				layerSelects[1] = false;

			}

		}



		void EFIKGroundingLayers::LoadLayers()

		{

			if (value == 0) {

				layerSelects[0] = true;

				for (int i = 1; i != layerTypeLabels.size(); ++i) {

					layerSelects[i] = false;

				}

			}

			else if (value == INT_MAX) {

				layerSelects[0] = false;

				for (int i = 1; i != layerTypeLabels.size(); ++i) {

					layerSelects[i] = true;

				}

			}

			else {

				layerSelects[0] = false;

				layerSelects[1] = false;

				for (int i = 2; i != layerTypeLabels.size(); ++i) {

					layerSelects[i] = ((EnumToInt_layer[i] & value) != 0);

				}

			}

		}







		EFIKGroundingUI::EFIKGroundingUI()

		{

			//layerSelects = (bool*)malloc(layerTypeLabels.size() * sizeof(bool));

			//memset(layerSelects, false, layerTypeLabels.size());

			Init();

		}



		EFIKGroundingUI::~EFIKGroundingUI()

		{

			//_ECHO_DELETE(m_pData);

			//memset(layerSelects, false, layerTypeLabels.size());

			//free(layerSelects);

		}



		void EFIKGroundingUI::Init()

		{

			Layers.SetShowType(UIShowType::TreeNode);

			Layers.SetLabel("Layers");

		}



		void EFIKGroundingUI::Show()

		{

			//bool IsUpdateLayers = false;

			Layers.ShowGui();

			IKDragFloat("Max Step", &MaxStep, 0.01f, 0.0001f, FLT_MAX);

			IKDragFloat("Height Offset", &HeightOffset, 0.01f, 0.f, FLT_MAX);

			IKDragFloat("Foot Speed", &FootSpeed, 0.01f, 0.0f, FLT_MAX);

			IKDragFloat("Foot Radius", &FootRadius, 0.01f, 0.0001f, 5.f);

			IKDragFloat("Foot Center Offset", &FootCenterOffset, 0.01f, FLT_MIN, FLT_MAX);

			IKDragFloat("Prediction", &Prediction, 0.01f, 0.0f, FLT_MAX);

			IKSliderFloat("Foot Rotation Weight", &FootRotationWeight);





			IKDragFloat("Foot Rotation Speed", &FootRotationSpeed, 0.01f, FLT_MIN, FLT_MAX);

			ImGui::Checkbox("Foot Rotation Free", &FootRotationFree);//新加的

			IKSliderFloat("Max Foot Rotation Angle", &MaxFootRotationAngle, 0.f, 90.f, "%.1f");

			ImGui::Checkbox("Rotate Solver", &RotateSolver);

			IKDragFloat("Pelvis Speed", &PelvisSpeed, 0.1f, FLT_MIN, FLT_MAX);

			IKSliderFloat("Pelvis Damper", &PelvisDamper);

			IKDragFloat("Lower Pelvis Weight", &LowerPelvisWeight, 0.01f, FLT_MIN, FLT_MAX);

			IKDragFloat("Lift Pelvis Weight", &LiftPelvisWeight, 0.01f, FLT_MIN, FLT_MAX);

			IKDragFloat("Root Sphere Cast Radius", &RootSphereCastRadius, 0.01f, 0.0001f, FLT_MAX);

			ImGui::Checkbox("Overstep Falls Down", &OverstepFallsDown);

			ImGui::Combo("Quality", &Quality, qualityTypeLabels.data(), (int)qualityTypeLabels.size());

			

		}



		void EFIKGroundingUI::Update()

		{

			if (!m_pData) {

				return;

			}

			m_pData->layers = (EchoIK::LayerMask)Layers.GetData();//enum



			m_pData->maxStep = MaxStep;

			m_pData->heightOffset = HeightOffset;

			m_pData->footSpeed = FootSpeed;

			m_pData->footRadius = FootRadius;

			m_pData->prediction = Prediction;

			m_pData->footRotationWeight = FootRotationWeight;

			m_pData->footRotationSpeed = FootRotationSpeed;

			m_pData->maxFootRotationAngle = MaxFootRotationAngle;

			m_pData->rotateSolver = RotateSolver;

			m_pData->pelvisSpeed = PelvisSpeed;

			m_pData->pelvisDamper = PelvisDamper;

			m_pData->lowerPelvisWeight = LowerPelvisWeight;

			m_pData->liftPelvisWeight = LiftPelvisWeight;

			m_pData->rootSphereCastRadius = RootSphereCastRadius;

			m_pData->overstepFallsDown = OverstepFallsDown;

			m_pData->quality = (EchoIK::Grounding::Quality)Quality;//enum



			m_pData->footRotationFree = FootRotationFree;

			m_pData->footCenterOffset = FootCenterOffset;



		}



		void EFIKGroundingUI::Load()

		{

			//if (!m_pData) {

			//	return;

			//}



			//Layers = m_pData->layers;//enum



			//LoadLayers();



			//MaxStep = m_pData->maxStep;

			//HeightOffset = m_pData->heightOffset;

			//FootSpeed = m_pData->footSpeed;

			//FootRadius = m_pData->footRadius;

			//Prediction = m_pData->prediction;

			//FootRotationWeight = m_pData->footRotationWeight;

			//FootRotationSpeed = m_pData->footRotationSpeed;

			//MaxFootRotationAngle = m_pData->maxFootRotationAngle;

			//RotateSolver = m_pData->rotateSolver;

			//PelvisSpeed = m_pData->pelvisSpeed;

			//PelvisDamper = m_pData->pelvisDamper;

			//LowerPelvisWeight = m_pData->lowerPelvisWeight;

			//LiftPelvisWeight = m_pData->liftPelvisWeight;

			//RootSphereCastRadius = m_pData->rootSphereCastRadius;

			//OverstepFallsDown = m_pData->overstepFallsDown;

			//Quality = m_pData->quality;//enum



			//FootRotationFree = m_pData->footRotationFree;

			//FootCenterOffset = m_pData->footCenterOffset;



		}



		void EFIKGroundingUI::DefaultInitialize() {



			Layers.SetData(2);

			MaxStep = 0.5f;

			HeightOffset = 0.0f;

			FootSpeed = 2.5f;

			FootRadius = 0.15f;

			FootCenterOffset = 0.0f;

			Prediction = 0.0f;

			FootRotationWeight = 1.0f;

			FootRotationSpeed = 7.0f;

			FootRotationFree = false;

			MaxFootRotationAngle = 45.0f;

			RotateSolver = false;

			PelvisSpeed = 5.0f;

			PelvisDamper = 0.0f;

			LowerPelvisWeight = 1.0f;

			LiftPelvisWeight = 0.0f;

			RootSphereCastRadius = 0.1f;

			OverstepFallsDown = true;

			Quality = 0;



		}



	};



	//IKEffector

	namespace EditorFinalIK {



		EFIKEffectorUI::EFIKEffectorUI() { Init(); }

		EFIKEffectorUI::~EFIKEffectorUI() {}



		void EFIKEffectorUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

// 			Target.SetShowType(UIShowType::None);

// 			Target.SetLabel("Target");

			Locator.SetShowType(UIShowType::None);

			Locator.SetLabel("srcLocator");

		}

		void EFIKEffectorUI::Show() {

			IKSliderFloat("Position Weight", &PositionWeight, 0.0f, 1.0f);

			ImGui::Checkbox("Use Thighs", &EffectChildNodes);

			IKSliderFloat("Rotation Weight", &RotationWeight, 0.0f, 1.0f);

			IKSliderFloat("Maintain Relative Pos", &MaintainRelativePositionWeight, 0.0f, 1.0f);

			Locator.ShowGui();

		}

		void EFIKEffectorUI::Update() {

		

			if (!m_pData) {

				return;

			}

			m_pData->positionWeight = PositionWeight;

			m_pData->effectChildNodes = EffectChildNodes;

			m_pData->rotationWeight = RotationWeight;

			m_pData->maintainRelativePositionWeight = MaintainRelativePositionWeight;

		}

		void EFIKEffectorUI::Load() {

		

// 			pTargetNode = nullptr;

// 			TargetPositionOffset = { 0,0,0 };

			//if (!m_pData) {

			//	return;

			//}



			////Target.SetNode(m_pData->target);

			//PositionWeight = m_pData->positionWeight;

			//EffectChildNodes = m_pData->effectChildNodes;

			//RotationWeight = m_pData->rotationWeight;

			//MaintainRelativePositionWeight = m_pData->maintainRelativePositionWeight;



		}



		void EFIKEffectorUI::DefaultInitialize() {



			PositionWeight = 0.0f;

			EffectChildNodes = true;

			RotationWeight = 0.0f;

			MaintainRelativePositionWeight = 0.0f;



		}



	};



	//FBIKChain

	namespace EditorFinalIK {



		EFIKFBIKChainUI::EFIKFBIKChainUI() { Init(); }

		EFIKFBIKChainUI::~EFIKFBIKChainUI() {}



		void EFIKFBIKChainUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			BendGoal.SetLabel("Bend Goal");

		}

		void EFIKFBIKChainUI::Show() {



			IKSliderFloat("Pull", &Pull, 0.0f, 1.0f);

			IKSliderFloat("Reach", &Reach, 0.0f, 1.0f);

			IKSliderFloat("Push", &Push, 0.0f, 1.0f);

			IKSliderFloat("Push Parent", &PushParent, -1.0f, 1.0f);



			ImGui::Combo("Reach Smoothing", &ReachSmoothing, SmoothingEnumLabel.data(), 3);

			ImGui::Combo("Push Smoothing", &PushSmoothing, SmoothingEnumLabel.data(), 3);



			BendGoal.ShowGui();

			IKSliderFloat("Bend Goal Weight", &BendGoalWeight, 0.0f, 1.0f);



		}

		void EFIKFBIKChainUI::Update() {

		

			if (!m_pData) {

				return;

			}

			m_pData->pull = Pull;

			m_pData->reach = Reach;

			m_pData->push = Push;

			m_pData->pushParent = PushParent;



			m_pData->reachSmoothing = (EchoIK::FBIKChain::Smoothing)ReachSmoothing;

			m_pData->pushSmoothing = (EchoIK::FBIKChain::Smoothing)PushSmoothing;



			//m_pData->bendConstraint->bendGoal = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(BendGoal.GetNodeName());

			m_pData->bendConstraint->weight = BendGoalWeight;



		}

		void EFIKFBIKChainUI::Load() {

		

			/*if (!m_pData) {

				return;

			}



			Pull = m_pData->pull;

			Reach = m_pData->reach;

			Push = m_pData->push;

			PushParent = m_pData->pushParent;



			ReachSmoothing = m_pData->reachSmoothing;

			PushSmoothing = m_pData->pushSmoothing;



			BendGoal.SetNode(m_pData->bendConstraint->bendGoal);

			BendGoalWeight = m_pData->bendConstraint->weight;*/



		}





		void EFIKFBIKChainUI::DefaultInitialize() {



			Pull = 1.0f;

			Reach = 0.1f;

			Push = 0.0f;

			PushParent = 0.0f;



			ReachSmoothing = 1;

			PushSmoothing = 1;



			BendGoal.SetNode(nullptr);

			BendGoalWeight = 0.0f;



		}





	};



	//EFIKMappingUI

	namespace EditorFinalIK {



		EFIKMappingLimbUI::EFIKMappingLimbUI() { Init(); }

		EFIKMappingLimbUI::~EFIKMappingLimbUI() {}



		void EFIKMappingLimbUI::Init() { SetShowType(UIShowType::TreeNodeEx_OpenOnArrow); }

		void EFIKMappingLimbUI::Show() {



			IKSliderFloat("Mapping Weight", &MappingWeight, 0.0f, 1.0f);

			IKSliderFloat("Maintain Hand Rot", &MaintainRotationWeight, 0.0f, 1.0f);

			

		}

		void EFIKMappingLimbUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->weight = MappingWeight;

			m_pData->maintainRotationWeight = MaintainRotationWeight;

		}

		void EFIKMappingLimbUI::Load() {

			/*if (!m_pData) {

				return;

			}

			MappingWeight = m_pData->weight;

			MaintainRotationWeight = m_pData->maintainRotationWeight;*/

		}

		void EFIKMappingLimbUI::DefaultInitialize() {



			MappingWeight = 1.0f;

			MaintainRotationWeight = 0.0f;

		}



		EFIKMappingSpineBoneUI::EFIKMappingSpineBoneUI() { Init(); }

		EFIKMappingSpineBoneUI::~EFIKMappingSpineBoneUI() {}



		void EFIKMappingSpineBoneUI::Init() { SetShowType(UIShowType::TreeNodeEx_OpenOnArrow); }

		void EFIKMappingSpineBoneUI::Show() {



			/*ImGui::SliderInt("Spine Iterations", &SpineIterations, 0, 3);*/

			IKSliderInt("Spine Iterations", &SpineIterations, 0, 3);

			IKSliderFloat("Spine Twist Weight", &SpineTwistWeight, 0.0f, 1.0f);

			IKSliderFloat("Maintain Head Rot", &MaintainRotationWeight, 0.0f, 1.0f);



		}

		void EFIKMappingSpineBoneUI::Update() {

			if (m_pDataSpine) {

				m_pDataSpine->iterations = SpineIterations;

				m_pDataSpine->twistWeight = SpineTwistWeight;

			}

			if (m_pDataHead) {

				m_pDataHead->maintainRotationWeight = MaintainRotationWeight;

			}

		}

		void EFIKMappingSpineBoneUI::Load() {

			/*if (m_pDataSpine) {

				SpineIterations = m_pDataSpine->iterations;

				SpineTwistWeight = m_pDataSpine->twistWeight;

			}

			if (m_pDataHead) {

				MaintainRotationWeight = m_pDataHead->maintainRotationWeight;

			}*/

		}

		void EFIKMappingSpineBoneUI::DefaultInitialize() {



			SpineIterations = 3;

			SpineTwistWeight = 1.0f;

			MaintainRotationWeight = 0.f;

		}

		void EFIKMappingSpineBoneUI::SetData(EchoIK::IKMappingSpine* pDataSpine, EchoIK::IKMappingBone* pDataHead) {

			m_pDataSpine = pDataSpine;

			m_pDataHead = pDataHead;

			Load();

		}

		EchoIK::IKMappingSpine* EFIKMappingSpineBoneUI::GetDataSpine() {

			return m_pDataSpine;

		}

		EchoIK::IKMappingBone* EFIKMappingSpineBoneUI::GetDataHead() {

			return m_pDataHead;

		}



	};



	//EFIKFullBodyBipedIKUI

	namespace EditorFinalIK {



		//--------------------------------------------------------------------------



		EFIKBipedReferencesUI::EFIKBipedReferencesUI() { Init(); }

		EFIKBipedReferencesUI::~EFIKBipedReferencesUI() {}



		void EFIKBipedReferencesUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



			Pelvis.SetLabel("Pelvis");

			LeftThigh.SetLabel("LeftThigh");

			LeftCalf.SetLabel("LeftCalf");

			LeftFoot.SetLabel("LeftFoot");

			RightThigh.SetLabel("RightThigh");

			RightCalf.SetLabel("RightCalf");

			RightFoot.SetLabel("RightFoot");

			LeftUpperArm.SetLabel("LeftUpperArm");

			LeftForearm.SetLabel("LeftForearm");

			LeftHand.SetLabel("LeftHand");

			RightUpperArm.SetLabel("RightUpperArm");

			RightForearm.SetLabel("RightForearm");

			RightHand.SetLabel("RightHand");

			Head.SetLabel("Head");

			Spine.SetLabel("Spine");

			Eyes.SetLabel("Eyes");





			Pelvis.SetShowType(UIShowType::None);

			LeftThigh.SetShowType(UIShowType::None);

			LeftCalf.SetShowType(UIShowType::None);

			LeftFoot.SetShowType(UIShowType::None);

			RightThigh.SetShowType(UIShowType::None);

			RightCalf.SetShowType(UIShowType::None);

			RightFoot.SetShowType(UIShowType::None);

			LeftUpperArm.SetShowType(UIShowType::None);

			LeftForearm.SetShowType(UIShowType::None);

			LeftHand.SetShowType(UIShowType::None);

			RightUpperArm.SetShowType(UIShowType::None);

			RightForearm.SetShowType(UIShowType::None);

			RightHand.SetShowType(UIShowType::None);

			Head.SetShowType(UIShowType::None);

			Spine.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			Eyes.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



		}

		void EFIKBipedReferencesUI::Show() {



			Pelvis.ShowGui();

			LeftThigh.ShowGui();

			LeftCalf.ShowGui();

			LeftFoot.ShowGui();

			RightThigh.ShowGui();

			RightCalf.ShowGui();

			RightFoot.ShowGui();

			LeftUpperArm.ShowGui();

			LeftForearm.ShowGui();

			LeftHand.ShowGui();

			RightUpperArm.ShowGui();

			RightForearm.ShowGui();

			RightHand.ShowGui();

			Head.ShowGui();

			Spine.ShowGui();

			Eyes.ShowGui();



		}

		void EFIKBipedReferencesUI::Update() {

			/*if (!m_pData) {

				return;

			}

			m_pData->root = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->m_pRoleNode;

			m_pData->pelvis = Pelvis.GetNode();

			m_pData->leftThigh = LeftThigh.GetNode();

			m_pData->leftCalf = LeftCalf.GetNode();

			m_pData->leftFoot = LeftFoot.GetNode();

			m_pData->rightThigh = RightThigh.GetNode();

			m_pData->rightCalf = RightCalf.GetNode();

			m_pData->rightFoot = RightFoot.GetNode();

			m_pData->leftUpperArm = LeftUpperArm.GetNode();

			m_pData->leftForearm = LeftForearm.GetNode();

			m_pData->leftHand = LeftHand.GetNode();

			m_pData->rightUpperArm = RightUpperArm.GetNode();

			m_pData->rightForearm = RightForearm.GetNode();

			m_pData->rightHand = RightHand.GetNode();

			m_pData->head = Head.GetNode();*/



			/*Spine.Update();

			Eyes.Update();*/



		}

		void EFIKBipedReferencesUI::Load() {

			if (m_pData) {

				Spine.SetData(&m_pData->spine);

				Eyes.SetData(&m_pData->eyes);

			}

			else {

				Spine.SetData(nullptr);

				Eyes.SetData(nullptr);

			}

			/*if (!m_pData) {

				return;

			}

			Pelvis.SetNode(m_pData->pelvis);

			LeftThigh.SetNode(m_pData->leftThigh);

			LeftCalf.SetNode(m_pData->leftCalf);

			LeftFoot.SetNode(m_pData->leftFoot);

			RightThigh.SetNode(m_pData->rightThigh);

			RightCalf.SetNode(m_pData->rightCalf);

			RightFoot.SetNode(m_pData->rightFoot);

			LeftUpperArm.SetNode(m_pData->leftUpperArm);

			LeftForearm.SetNode(m_pData->leftForearm);

			LeftHand.SetNode(m_pData->leftHand);

			RightUpperArm.SetNode(m_pData->rightUpperArm);

			RightForearm.SetNode(m_pData->rightForearm);

			RightHand.SetNode(m_pData->rightHand);

			Head.SetNode(m_pData->head);



			Spine.SetData(&m_pData->spine);

			Eyes.SetData(&m_pData->eyes);*/



		}



		void EFIKBipedReferencesUI::DefaultInitialize() {



			RoleIKComponent* pRoleIKComponent = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent();



			Pelvis.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Pelvis));

			LeftThigh.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_LThigh));

			LeftCalf.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_LCalf));

			LeftFoot.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_LFoot));

			RightThigh.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_RThigh));

			RightCalf.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_RCalf));

			RightFoot.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_RFoot));

			LeftUpperArm.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_LUpperArm));

			LeftForearm.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_LForearm));

			LeftHand.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_LHand));

			RightUpperArm.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_RUpperArm));

			RightForearm.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_RForearm));

			RightHand.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_RHand));

			Head.SetNode(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Head));



			Spine.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine1));

			Spine.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine2));





		}



		UINT32 EFIKBipedReferencesUI::JudgeCanUseComponent() {

			bool IsCanUse = true;

			IsCanUse = IsCanUse && Pelvis.GetNode() != nullptr;

			IsCanUse = IsCanUse && LeftThigh.GetNode() != nullptr;

			IsCanUse = IsCanUse && LeftCalf.GetNode() != nullptr;

			IsCanUse = IsCanUse && LeftFoot.GetNode() != nullptr;

			IsCanUse = IsCanUse && RightThigh.GetNode() != nullptr;

			IsCanUse = IsCanUse && RightCalf.GetNode() != nullptr;

			IsCanUse = IsCanUse && RightFoot.GetNode() != nullptr;

			IsCanUse = IsCanUse && LeftUpperArm.GetNode() != nullptr;

			IsCanUse = IsCanUse && LeftForearm.GetNode() != nullptr;

			IsCanUse = IsCanUse && LeftHand.GetNode() != nullptr;

			IsCanUse = IsCanUse && RightUpperArm.GetNode() != nullptr;

			IsCanUse = IsCanUse && RightForearm.GetNode() != nullptr;

			IsCanUse = IsCanUse && RightHand.GetNode() != nullptr;

			IsCanUse = IsCanUse && Head.GetNode() != nullptr;

			IsCanUse = IsCanUse && Spine.size() > 1;

			if (IsCanUse) {

				return UIErrorType_NoError;

			}

			else {

				return UIErrorType_ParametersAreNotFullySet;

			}

		}



		//--------------------------------------------------------------------------



		EFIKSolverFullBodyBipedUI::EFIKSolverFullBodyBipedUI() { Init(); }

		EFIKSolverFullBodyBipedUI::~EFIKSolverFullBodyBipedUI() {}



		void EFIKSolverFullBodyBipedUI::Init() {

		

			SetShowType(UIShowType::None);

		

			BodyEffector.ikType = Echo::IKEffectorType::IK_Body;

			LeftHandEffector.ikType = Echo::IKEffectorType::IK_LeftHand;

			LeftShoulderEffector.ikType = Echo::IKEffectorType::IK_LeftShoulder;

			RightHandEffector.ikType = Echo::IKEffectorType::IK_RightHand;

			RightShoulderEffector.ikType = Echo::IKEffectorType::IK_RightShoulder;

			LeftFootEffector.ikType = Echo::IKEffectorType::IK_LeftFoot;

			LeftThighEffector.ikType = Echo::IKEffectorType::IK_LeftThigh;

			RightFootEffector.ikType = Echo::IKEffectorType::IK_RightFoot;

			RightThighEffector.ikType = Echo::IKEffectorType::IK_RightThigh;





		

			BodyEffector.SetLabel("Body Effector");

			LeftHandEffector.SetLabel("Left Hand Effector");

			LeftShoulderEffector.SetLabel("Left Shoulder Effector");

			RightHandEffector.SetLabel("Right Hand Effector");

			RightShoulderEffector.SetLabel("Right Shoulder Effector");

			LeftFootEffector.SetLabel("Left Foot Effector");

			LeftThighEffector.SetLabel("Left Thigh Effector");

			RightFootEffector.SetLabel("Right Foot Effector");

			RightThighEffector.SetLabel("Right Thigh Effector");

		



			LeftArmChain.SetLabel("Chain##LeftArm");

			RightArmChain.SetLabel("Chain##RightArm");

			LeftLegChain.SetLabel("Chain##LeftLeg");

			RightLegChain.SetLabel("Chain##RightLeg");



			BodyMapping.SetLabel("Mapping##Body");

			LeftArmMapping.SetLabel("Mapping##LeftArm");

			RightArmMapping.SetLabel("Mapping##RightArm");

			LeftLegMapping.SetLabel("Mapping##LeftLeg");

			RightLegMapping.SetLabel("Mapping##RightLeg");





			BodyEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftHandEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftShoulderEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightHandEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightShoulderEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftFootEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftThighEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightFootEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightThighEffector.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

		

			LeftArmChain.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightArmChain.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftLegChain.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightLegChain.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);





			BodyMapping.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftArmMapping.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightArmMapping.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftLegMapping.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightLegMapping.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);





		}

		void EFIKSolverFullBodyBipedUI::Show() {

			

			if (ImGui::TreeNodeEx("Body", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

				BodyEffector.ShowGui();

				if (ImGui::TreeNodeEx("Chain##Body", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

					IKSliderFloat("Spine Stiffness", &SpineStiffness, 0.0f, 1.0f);

					IKSliderFloat("Pull Body Vertical", &PullBodyVertical, -1.0f, 1.0f);

					IKSliderFloat("Pull Body Horizontal", &PullBodyHorizontal, -1.0f, 1.0f);

					ImGui::TreePop();

				}

				BodyMapping.ShowGui();

				ImGui::TreePop();

			}

			if (ImGui::TreeNodeEx("Left Arm", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

				LeftHandEffector.ShowGui();

				LeftShoulderEffector.ShowGui();

				LeftArmChain.ShowGui();

				LeftArmMapping.ShowGui();

				ImGui::TreePop();

			}

			if (ImGui::TreeNodeEx("Right Arm", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

				RightHandEffector.ShowGui();

				RightShoulderEffector.ShowGui();

				RightArmChain.ShowGui();

				RightArmMapping.ShowGui();

				ImGui::TreePop();

			}

			if (ImGui::TreeNodeEx("Left Leg", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

				LeftFootEffector.ShowGui();

				LeftThighEffector.ShowGui();

				LeftLegChain.ShowGui();

				LeftLegMapping.ShowGui();

				ImGui::TreePop();

			}

			if (ImGui::TreeNodeEx("Right Leg", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

				RightFootEffector.ShowGui();

				RightThighEffector.ShowGui();

				RightLegChain.ShowGui();

				RightLegMapping.ShowGui();

				ImGui::TreePop();

			}

		}

		void EFIKSolverFullBodyBipedUI::Update() {

		

			if (!m_pData) {

				return;

			}

			BodyEffector.Update();

			LeftHandEffector.Update();

			LeftShoulderEffector.Update();

			RightHandEffector.Update();

			RightShoulderEffector.Update();

			LeftFootEffector.Update();

			LeftThighEffector.Update();

			RightFootEffector.Update();

			RightThighEffector.Update();

			LeftArmChain.Update();

			RightArmChain.Update();

			LeftLegChain.Update();

			RightLegChain.Update();

			BodyMapping.Update();

			LeftArmMapping.Update();

			RightArmMapping.Update();

			LeftLegMapping.Update();

			RightLegMapping.Update();



			m_pData->spineStiffness = SpineStiffness;

			m_pData->pullBodyVertical = PullBodyVertical;

			m_pData->pullBodyHorizontal = PullBodyHorizontal;

		

		}

		void EFIKSolverFullBodyBipedUI::Load() {

		

			if (m_pData) {

				if (m_pData->effectors.size() > 8) {

					BodyEffector.SetData(m_pData->GetBodyEffector());

					LeftHandEffector.SetData(m_pData->GetLeftHandEffector());

					LeftShoulderEffector.SetData(m_pData->GetLeftShoulderEffector());

					RightHandEffector.SetData(m_pData->GetRightHandEffector());

					RightShoulderEffector.SetData(m_pData->GetRightShoulderEffector());

					LeftFootEffector.SetData(m_pData->GetLeftFootEffector());

					LeftThighEffector.SetData(m_pData->GetLeftThighEffector());

					RightFootEffector.SetData(m_pData->GetRightFootEffector());

					RightThighEffector.SetData(m_pData->GetRightThighEffector());

				}



				if (m_pData->chain.size() > 4) {

					LeftArmChain.SetData(m_pData->GetLeftArmChain());

					RightArmChain.SetData(m_pData->GetRightArmChain());

					LeftLegChain.SetData(m_pData->GetLeftLegChain());

					RightLegChain.SetData(m_pData->GetRightLegChain());

				}



				if (m_pData->boneMappings.size() > 1) {

					BodyMapping.SetData(m_pData->GetSpineMapping(), m_pData->GetHeadMapping());

				}

				if (m_pData->limbMappings.size() > 3) {

					LeftArmMapping.SetData(m_pData->GetLeftArmMapping());

					RightArmMapping.SetData(m_pData->GetRightArmMapping());

					LeftLegMapping.SetData(m_pData->GetLeftLegMapping());

					RightLegMapping.SetData(m_pData->GetRightLegMapping());

				}

			}

			else {

				BodyEffector.SetData(nullptr);

				LeftHandEffector.SetData(nullptr);

				LeftShoulderEffector.SetData(nullptr);

				RightHandEffector.SetData(nullptr);

				RightShoulderEffector.SetData(nullptr);

				LeftFootEffector.SetData(nullptr);

				LeftThighEffector.SetData(nullptr);

				RightFootEffector.SetData(nullptr);

				RightThighEffector.SetData(nullptr);

				

				LeftArmChain.SetData(nullptr);

				RightArmChain.SetData(nullptr);

				LeftLegChain.SetData(nullptr);

				RightLegChain.SetData(nullptr);

				

				BodyMapping.SetData(nullptr,nullptr);

				

				LeftArmMapping.SetData(nullptr);

				RightArmMapping.SetData(nullptr);

				LeftLegMapping.SetData(nullptr);

				RightLegMapping.SetData(nullptr);

				

			}

			

			



			

			//if (!m_pData) {

			//	return;

			//}

			////将数据传到对应的UI中

			//if (m_pData->effectors.size() > 8) {

			//	BodyEffector.SetData(m_pData->GetBodyEffector());

			//	LeftHandEffector.SetData(m_pData->GetLeftHandEffector());

			//	LeftShoulderEffector.SetData(m_pData->GetLeftShoulderEffector());

			//	RightHandEffector.SetData(m_pData->GetRightHandEffector());

			//	RightShoulderEffector.SetData(m_pData->GetRightShoulderEffector());

			//	LeftFootEffector.SetData(m_pData->GetLeftFootEffector());

			//	LeftThighEffector.SetData(m_pData->GetLeftThighEffector());

			//	RightFootEffector.SetData(m_pData->GetRightFootEffector());

			//	RightThighEffector.SetData(m_pData->GetRightThighEffector());

			//}

			//

			//if (m_pData->chain.size() > 4) {

			//	LeftArmChain.SetData(m_pData->GetLeftArmChain());

			//	RightArmChain.SetData(m_pData->GetRightArmChain());

			//	LeftLegChain.SetData(m_pData->GetLeftLegChain());

			//	RightLegChain.SetData(m_pData->GetRightLegChain());

			//}

			//

			//if (m_pData->boneMappings.size() > 1) {

			//	BodyMapping.SetData(m_pData->GetSpineMapping(), m_pData->GetHeadMapping());

			//}

			//if (m_pData->limbMappings.size() > 3) {

			//	LeftArmMapping.SetData(m_pData->GetLeftArmMapping());

			//	RightArmMapping.SetData(m_pData->GetRightArmMapping());

			//	LeftLegMapping.SetData(m_pData->GetLeftLegMapping());

			//	RightLegMapping.SetData(m_pData->GetRightLegMapping());

			//}



			//SpineStiffness = m_pData->spineStiffness;

			//PullBodyVertical = m_pData->pullBodyVertical;

			//PullBodyHorizontal = m_pData->pullBodyHorizontal;



		}



		void EFIKSolverFullBodyBipedUI::DefaultInitialize() {



			BodyEffector.DefaultInitialize();

			LeftHandEffector.DefaultInitialize();

			LeftShoulderEffector.DefaultInitialize();

			RightHandEffector.DefaultInitialize();

			RightShoulderEffector.DefaultInitialize();

			LeftFootEffector.DefaultInitialize();

			LeftThighEffector.DefaultInitialize();

			RightFootEffector.DefaultInitialize();

			RightThighEffector.DefaultInitialize();

			LeftArmChain.DefaultInitialize();

			RightArmChain.DefaultInitialize();

			LeftLegChain.DefaultInitialize();

			RightLegChain.DefaultInitialize();

			BodyMapping.DefaultInitialize();

			LeftArmMapping.DefaultInitialize();

			RightArmMapping.DefaultInitialize();

			LeftLegMapping.DefaultInitialize();

			RightLegMapping.DefaultInitialize();



			SpineStiffness = 0.5f;

			PullBodyVertical = 0.5f;

			PullBodyHorizontal = 0.0f;



		}



		UINT32 EFIKSolverFullBodyBipedUI::JudgeCanUseComponent() {

			return UIErrorType_NoError;

		}



		//--------------------------------------------------------------------------



		EFIKFullBodyBipedIKUI::EFIKFullBodyBipedIKUI() { Init(); }

		//EFIKFullBodyBipedIKUI::EFIKFullBodyBipedIKUI(RoleObject* pRoleObject){

		//	Init();

		//	SetLabel("FullBodyBipedIK##" + pRoleObject->getName().toString());

		//	//m_pData = new EchoIK::FullBodyBipedIK(pRoleObject->getRoleIKComponent());

		//	m_pData = (EchoIK::FullBodyBipedIK*)pRoleObject->getRoleIKComponent()->CreateIKComponentImpl(Name("FullBodyBipedIK"), Name("FullBodyBipedIK"), FinalIKTypeEnum::FinalIKType_FullBodyBipedIK);

		//	m_pData->disable();

		//	SetData(m_pData);

		//	EditorFinalIKUI::instance()->GetCurUI()->AddPtrToPool(this);

		//}

		EFIKFullBodyBipedIKUI::~EFIKFullBodyBipedIKUI() {}



		void EFIKFullBodyBipedIKUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_Del);

			References.SetLabel("References");

		}

		void EFIKFullBodyBipedIKUI::PureShow() {



			ImGui::Checkbox("Fix Transforms", &FixTransforms);

			References.ShowGui();

			

			IKSliderFloat("Weight", &Weight, 0.0f, 1.0f);

			//ImGui::SliderInt("Iterations", &Iterations, 0, 10);

			IKSliderInt("Iterations", &Iterations, 0, 10);

			Solver.ShowGui();

		}

		void EFIKFullBodyBipedIKUI::Update() {

			if (!m_pData) {

				return;

			}

			References.Update();

			Solver.Update();

			m_pData->GetIKSolver()->iterations = Iterations;

			m_pData->GetIKSolver()->IKPositionWeight = Weight;

			m_pData->fixTransforms = FixTransforms;



			//m_pData->solver->SetToReferences(References.GetData(), NULL);

			ToUpdateAllFBBIK();

		}

		void EFIKFullBodyBipedIKUI::Load() {



			if (m_pData) {

				References.SetData(m_pData->references);

				Solver.SetData(m_pData->solver);

			}

			else {

				References.SetData(nullptr);

				Solver.SetData(nullptr);

			}



			/*if (!m_pData) {

				return;

			}



			References.SetData(m_pData->references);

			Solver.SetData(m_pData->solver);



			Iterations = m_pData->GetIKSolver()->iterations;

			Weight = m_pData->GetIKSolver()->IKPositionWeight;

			FixTransforms = m_pData->fixTransforms;*/



		}



		UINT32 EFIKFullBodyBipedIKUI::JudgeCanUseComponent() {

			UINT32 ref = References.JudgeCanUseComponent();

			if (ref != UIErrorType_NoError) {

				return ref;

			}

			return Solver.JudgeCanUseComponent();

		}



		void EFIKFullBodyBipedIKUI::DefaultInitialize() {



			FixTransforms = false;

			References.DefaultInitialize();

			Weight = 1.0f;

			Iterations = 0;

			Solver.DefaultInitialize();



		}



		 void EFIKFullBodyBipedIKUI::PreResetComponent(bool IsUse) { }

		 void EFIKFullBodyBipedIKUI::PostResetComponent(bool IsUse) {

			 if (IsUse) {

				 m_pData->disable();

			 }

		 }



		 void EFIKFullBodyBipedIKUI::ToUpdateAllFBBIK() {

			 if (!m_pData) {

				 return;

			 }

			 std::vector<EchoIK::FullBodyBipedIK*> fbbIKs;

			 EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->GetFullBodyBipedIK(Name(GetLabel()), fbbIKs);



			 int n = 0;

			 for (auto fbbik : fbbIKs) {

				 if (fbbik == m_pData) {

					 continue;

				 }



				 fbbik->fixTransforms = m_pData->fixTransforms;

				 fbbik->GetIKSolver()->iterations = m_pData->GetIKSolver()->iterations;

				 fbbik->GetIKSolver()->IKPositionWeight = m_pData->GetIKSolver()->IKPositionWeight;



				 //EFIKBipedReferencesUI

				 fbbik->references->pelvis = m_pData->references->pelvis;

				 fbbik->references->leftThigh = m_pData->references->leftThigh;

				 fbbik->references->leftCalf = m_pData->references->leftCalf;

				 fbbik->references->leftFoot = m_pData->references->leftFoot;

				 fbbik->references->rightThigh = m_pData->references->rightThigh;

				 fbbik->references->rightCalf = m_pData->references->rightCalf;

				 fbbik->references->rightFoot = m_pData->references->rightFoot;

				 fbbik->references->leftUpperArm = m_pData->references->leftUpperArm;

				 fbbik->references->leftForearm = m_pData->references->leftForearm;

				 fbbik->references->leftHand = m_pData->references->leftHand;

				 fbbik->references->rightUpperArm = m_pData->references->rightUpperArm;

				 fbbik->references->rightForearm = m_pData->references->rightForearm;

				 fbbik->references->rightHand = m_pData->references->rightHand;

				 fbbik->references->head = m_pData->references->head;



				 int size = (int)m_pData->references->spine.size();

				 fbbik->references->spine.resize(size);

				 for (int i = 0; i != size; ++i) {

					 fbbik->references->spine[i] = m_pData->references->spine[i];

				 }

				 size = (int)m_pData->references->eyes.size();

				 fbbik->references->eyes.resize(size);

				 for (int i = 0; i != size; ++i) {

					 fbbik->references->eyes[i] = m_pData->references->eyes[i];

				 }



				 //EFIKSolverFullBodyBipedUI

				 fbbik->solver->spineStiffness = m_pData->solver->spineStiffness;

				 fbbik->solver->pullBodyVertical = m_pData->solver->pullBodyVertical;

				 fbbik->solver->pullBodyHorizontal = m_pData->solver->pullBodyHorizontal;



				 auto UpdateEffector = [&](EchoIK::IKEffector* pSrc, EchoIK::IKEffector* pDst, unsigned int Mesh = 31) {



					 if (Mesh & 2)	pDst->positionWeight = pSrc->positionWeight;

					 if (Mesh & 4)	pDst->effectChildNodes = pSrc->effectChildNodes;

					 if (Mesh & 8)	pDst->rotationWeight = pSrc->rotationWeight;

					 if (Mesh & 16) pDst->maintainRelativePositionWeight = pSrc->maintainRelativePositionWeight;



				 };

				 auto UpdateFBIKChain = [&](EchoIK::FBIKChain* pSrc, EchoIK::FBIKChain* pDst) {

				 

					 pDst->pull = pSrc->pull;

					 pDst->reach = pSrc->reach;

					 pDst->push = pSrc->push;

					 pDst->pushParent = pSrc->pushParent;

					 pDst->reachSmoothing = pSrc->reachSmoothing;

					 pDst->pushSmoothing = pSrc->pushSmoothing;

					 pDst->bendConstraint->weight = pSrc->bendConstraint->weight;



				 };

				 auto UpdateMappingLimb = [&](EchoIK::IKMappingLimb* pSrc, EchoIK::IKMappingLimb* pDst) {

				 

					 pDst->weight = pSrc->weight;

					 pDst->maintainRotationWeight = pSrc->maintainRotationWeight;



				 };

				 UpdateEffector(m_pData->solver->GetBodyEffector(), fbbik->solver->GetBodyEffector(), 7);

				 UpdateEffector(m_pData->solver->GetLeftHandEffector(), fbbik->solver->GetLeftHandEffector(), 27);

				 UpdateEffector(m_pData->solver->GetLeftShoulderEffector(), fbbik->solver->GetLeftShoulderEffector(), 3);

				 UpdateEffector(m_pData->solver->GetRightHandEffector(), fbbik->solver->GetRightHandEffector(), 27);

				 UpdateEffector(m_pData->solver->GetRightShoulderEffector(), fbbik->solver->GetRightShoulderEffector(), 3);

				 UpdateEffector(m_pData->solver->GetLeftFootEffector(), fbbik->solver->GetLeftFootEffector(), 27);

				 UpdateEffector(m_pData->solver->GetLeftThighEffector(), fbbik->solver->GetLeftThighEffector(), 3);

				 UpdateEffector(m_pData->solver->GetRightFootEffector(), fbbik->solver->GetRightFootEffector(), 27);

				 UpdateEffector(m_pData->solver->GetRightThighEffector(), fbbik->solver->GetRightThighEffector(), 3);



				 UpdateFBIKChain(m_pData->solver->GetLeftArmChain(), fbbik->solver->GetLeftArmChain());

				 UpdateFBIKChain(m_pData->solver->GetRightArmChain(), fbbik->solver->GetRightArmChain());

				 UpdateFBIKChain(m_pData->solver->GetLeftLegChain(), fbbik->solver->GetLeftLegChain());

				 UpdateFBIKChain(m_pData->solver->GetRightLegChain(), fbbik->solver->GetRightLegChain());



				 UpdateMappingLimb(m_pData->solver->GetLeftArmMapping(), fbbik->solver->GetLeftArmMapping());

				 UpdateMappingLimb(m_pData->solver->GetRightArmMapping(), fbbik->solver->GetRightArmMapping());

				 UpdateMappingLimb(m_pData->solver->GetLeftLegMapping(), fbbik->solver->GetLeftLegMapping());

				 UpdateMappingLimb(m_pData->solver->GetRightLegMapping(), fbbik->solver->GetRightLegMapping());



				 fbbik->solver->GetSpineMapping()->iterations = m_pData->solver->GetSpineMapping()->iterations;

				 fbbik->solver->GetSpineMapping()->twistWeight = m_pData->solver->GetSpineMapping()->twistWeight;

				 fbbik->solver->GetHeadMapping()->maintainRotationWeight = m_pData->solver->GetHeadMapping()->maintainRotationWeight;

			 }

		 }



		//--------------------------------------------------------------------------



		FBBIKSelectUI::FBBIKSelectUI() {

			PopupLabel = std::string("SelectFBBIK##") + PtrToString(this);

			SetIsJudge(true);

		}

		FBBIKSelectUI::~FBBIKSelectUI() {}

		void FBBIKSelectUI::OnButton2(bool IsClick) {



			const std::map<Name, EFIKFullBodyBipedIKUI*>* mp = EditorFinalIKUI::instance()->GetCurUI()->GetFBBIKMap();

			EFIKFullBodyBipedIKUI* selItem = m_pData;

			if (OnButton2UseMap(IsClick, mp, PopupLabel, &selItem)) {

				SetData(selItem);

			}

		

		}

		void FBBIKSelectUI::MessageResponse() {}

		void FBBIKSelectUI::OnButton1(bool IsClick) {

			if (!EditorFinalIKUI::instance()->GetCurUI()->PtrIsExist(m_pData)) {

				SetData(nullptr);

			}

			if (IsClick && m_pData) {

				ImGui::OpenPopup((std::string("ShowNode##") + GetLabel()).c_str());

			}

			//ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth() * 0.5, ImGui::GetWindowHeight() * 0.5));

			if (ImGui::BeginPopup((std::string("ShowNode##") + GetLabel()).c_str())) {

				m_pData->ShowGui(UIShowType::None, 0.9f);

				ImGui::EndPopup();

			}

		}

		void FBBIKSelectUI::SetData(EFIKFullBodyBipedIKUI* pData) {

			m_pData = pData;

			if (m_pData) {

				SetSelectName(m_pData->GetLabel());

			}

			else {

				SetSelectName("None");

			}

		}

		EchoIK::FullBodyBipedIK* FBBIKSelectUI::GetFBBIK() {

			if (!m_pData) {

				return nullptr;

			}

			return m_pData->GetData();

		}

		const char* FBBIKSelectUI::GetFBBIKName() {

			if (!m_pData) {

				return "";

			}

			return m_pData->GetLabel();

		}

		

		const ImVec4& FBBIKSelectUI::ButtonColor() {

			if (m_pData) {

				return DefaultButtonColor;

			}

			return FBBIKSelectButtonColor;

		}

		//--------------------------------------------------------------------------

	};



	//EFIKGrounderQuadrupedUI

	namespace EditorFinalIK {



		EFIKGrounderQuadrupedUI::EFIKGrounderQuadrupedUI()

		{

			Init();

		}



		/*EFIKGrounderQuadrupedUI::EFIKGrounderQuadrupedUI(RoleObject * pRoleObject)

			: EFIKComponentBase(pRoleObject)

		{

			SetLabel("GrounderQuadruped##" + pRoleObject->getName().toString());

			Init();

			Load();

		}*/



		EFIKGrounderQuadrupedUI::~EFIKGrounderQuadrupedUI() {}



		void EFIKGrounderQuadrupedUI::Init()

		{



			mSolver.SetLabel("Solver");

			mForelegSolver.SetLabel("Foreleg Solver");

			mLegs.SetLabel("Legs");

			mForelegs.SetLabel("Forelegs");



			mPelvis.SetLabel("Pelvis");

			mLastSpineBone.SetLabel("Last Spine Bone");

			mHead.SetLabel("Head");





			mSolver.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			mForelegSolver.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			mPelvis.SetShowType(UIShowType::None);

			mLastSpineBone.SetShowType(UIShowType::None);

			mHead.SetShowType(UIShowType::None);

			mLegs.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			mForelegs.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



		}

		void EFIKGrounderQuadrupedUI::PureShow()

		{





			IKSliderFloat("Weight", &Weight, 0.0f, 1.0f);



			mSolver.ShowGui();

			mForelegSolver.ShowGui();



			IKSliderFloat("Root Rotation Weight", &RootRotationWeight, 0.f, 1.f);

			IKSliderFloat("Min Root Rotation", &MinRootRotation, -90.f, 0.f, "%.1f");

			IKSliderFloat("Max Root Rotation", &MaxRootRotation, 0.f, 90.f, "%.1f");

			IKDragFloat("Root Rotation Speed", &RootRotationSpeed, 0.1f, 0.0f, FLT_MAX);

			IKDragFloat("Max Leg Offset", &MaxLegOffset, 0.1f, 0.0f, FLT_MAX);

			IKDragFloat("Max Fore Leg Offset", &MaxForeLegOffset, 0.1f, 0.0f, FLT_MAX);

			IKSliderFloat("Maintain Head Rotation Weight", &MaintainHeadRotationWeight, 0.f, 1.f);



			mPelvis.ShowGui();

			mLastSpineBone.ShowGui();

			mHead.ShowGui();

			mLegs.ShowGui();

			mForelegs.ShowGui();





		}



		void EFIKGrounderQuadrupedUI::Update()

		{

			if (!m_pData) {

				return;

			}

			m_pData->weight = Weight;

			m_pData->rootRotationWeight = RootRotationWeight;

			m_pData->minRootRotation = MinRootRotation;

			m_pData->maxRootRotation = MaxRootRotation;

			m_pData->rootRotationSpeed = RootRotationSpeed;

			m_pData->maxLegOffset = MaxLegOffset;

			m_pData->maxForeLegOffset = MaxForeLegOffset;

			m_pData->maintainHeadRotationWeight = MaintainHeadRotationWeight;

			//m_pData->pelvis = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(mPelvis.GetNodeName());

			//m_pData->lastSpineBone = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(mLastSpineBone.GetNodeName());

			//m_pData->head = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(mHead.GetNodeName());





			mSolver.Update();

			mForelegSolver.Update();

			//mSolver.GetData()->m_rootBone = m_pData->pelvis;

			//mForelegSolver.GetData()->m_rootBone = m_pData->pelvis;



			

			//mLegs.Update();

			//mForelegs.Update();



		}



		void EFIKGrounderQuadrupedUI::Load()

		{

			if (m_pData) {

				mSolver.SetData(m_pData->solver);

				mForelegSolver.SetData(m_pData->forelegSolver);

				mLegs.SetData(&m_pData->legs);

				mForelegs.SetData(&m_pData->forelegs);

			}

			else {

				mSolver.SetData(nullptr);

				mForelegSolver.SetData(nullptr);

				mLegs.SetData(nullptr);

				mForelegs.SetData(nullptr);

			}



			/*if (!m_pData) {

				return;

			}



			Weight = m_pData->weight;

			RootRotationWeight = m_pData->rootRotationWeight;

			MinRootRotation = m_pData->minRootRotation;

			MaxRootRotation = m_pData->maxRootRotation;

			RootRotationSpeed = m_pData->rootRotationSpeed;

			MaxLegOffset = m_pData->maxLegOffset;

			MaxForeLegOffset = m_pData->maxForeLegOffset;

			MaintainHeadRotationWeight = m_pData->maintainHeadRotationWeight;



			mPelvis.SetNode((Bone*)m_pData->pelvis);

			mLastSpineBone.SetNode((Bone*)m_pData->lastSpineBone);

			mHead.SetNode((Bone*)m_pData->head);



			mSolver.SetData(m_pData->solver);

			mForelegSolver.SetData(m_pData->forelegSolver);



			mLegs.SetData(&m_pData->legs);

			mForelegs.SetData(&m_pData->forelegs);



			UseComponent(false);*/

		}



		UINT32 EFIKGrounderQuadrupedUI::JudgeCanUseComponent() {

			bool IsCanUse = true;

			IsCanUse = IsCanUse && mPelvis.GetNode() != nullptr;

			IsCanUse = IsCanUse && mLastSpineBone.GetNode() != nullptr;

			IsCanUse = IsCanUse && mHead.GetNode() != nullptr;

			

			if (!IsCanUse) {

				return UIErrorType_ParametersAreNotFullySet;

			}



			for (int i = 0; i != mLegs.size(); ++i) {

				auto pLimb = mLegs.At(i);

				if (pLimb) {

					IsCanUse = IsCanUse && pLimb->JudgeCanUseComponent() == UIErrorType_NoError;

				}

				else {

					return UIErrorType_ParametersAreNotFullySet;

				}

			}



			for (int i = 0; i != mForelegs.size(); ++i) {

				auto pLimb = mForelegs.At(i);

				if (pLimb) {

					IsCanUse = IsCanUse && pLimb->JudgeCanUseComponent() == UIErrorType_NoError;

				}

				else {

					return UIErrorType_ParametersAreNotFullySet;

				}

			}

			return IsCanUse? UIErrorType_NoError : UIErrorType_ChildrenParametersAreNotFullySet;

		}



		void EFIKGrounderQuadrupedUI::DefaultInitialize() {



			Weight = 1.0f;

			RootRotationWeight = 1.0f;

			MinRootRotation = -25.0f;

			MaxRootRotation = 45.0f;



			RootRotationSpeed = 5.0f;

			MaxLegOffset = 0.5f;

			MaxForeLegOffset = 0.5f;

			MaintainHeadRotationWeight = 0.5f;



			mSolver.DefaultInitialize();

			mForelegSolver.DefaultInitialize();

			//EditorLimbGroupUI mLegs;

			//EditorLimbGroupUI mForelegs;



			RoleIKComponent* pRoleIKComponent = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent();



			mPelvis.SetNode(pRoleIKComponent->getBoneNodeByType(Quadruped_Bone_Pelvis,1));

			mLastSpineBone.SetNode(pRoleIKComponent->getBoneNodeByType(Quadruped_Bone_LastSpine,1));

			mHead.SetNode(pRoleIKComponent->getBoneNodeByType(Quadruped_Bone_Head,1));



		}



		void EFIKGrounderQuadrupedUI::PreResetComponent(bool IsUse) {

			if (IsUse) {

				for (int i = 0; i != mLegs.size(); ++i) {

					auto pLimb = mLegs.At(i);

					if (pLimb) {

						pLimb->CallUseComponent(IsUse);

					}

				}



				for (int i = 0; i != mForelegs.size(); ++i) {

					auto pLimb = mForelegs.At(i);

					if (pLimb) {

						pLimb->CallUseComponent(IsUse);

					}

				}

			}

		}

		void EFIKGrounderQuadrupedUI::PostResetComponent(bool IsUse) {

			if (!IsUse) {

				for (int i = 0; i != mLegs.size(); ++i) {

					auto pLimb = mLegs.At(i);

					if (pLimb) {

						pLimb->CallUseComponent(IsUse);

					}

				}



				for (int i = 0; i != mForelegs.size(); ++i) {

					auto pLimb = mForelegs.At(i);

					if (pLimb) {

						pLimb->CallUseComponent(IsUse);

					}

				}

			}

		}

	};



	//EFIKGrounderIKUI

	namespace EditorFinalIK {

		EFIKGrounderIKUI::EFIKGrounderIKUI() {

			Init();

		}

		/*EFIKGrounderIKUI::EFIKGrounderIKUI(RoleObject * pRoleObject)

			: EFIKComponentBase(pRoleObject)

		{

			SetLabel("GrounderIK##" + pRoleObject->getName().toString());

			Init();

			Load();

		}*/

		EFIKGrounderIKUI::~EFIKGrounderIKUI() { }



		void EFIKGrounderIKUI::Init() {



			mSolver.SetLabel("Solver");

			mLegs.SetLabel("Legs");



			mPelvis.SetLabel("Pelvis");





			mSolver.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			mPelvis.SetShowType(UIShowType::None);

			mLegs.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



		}

		void EFIKGrounderIKUI::PureShow() {







			IKSliderFloat("Weight", &Weight, 0.0f, 1.0f);



			mSolver.ShowGui();



			IKSliderFloat("Root Rotation Weight", &RootRotationWeight, 0, 1.f);

			IKDragFloat("Root Rotation Speed", &RootRotationSpeed, 0.01f, 0.0f, FLT_MAX);

			IKDragFloat("Max Root Rotation Angle", &MaxRootRotationAngle, 0.1f, 0.0f, 90.0f);





			mPelvis.ShowGui();

			mLegs.ShowGui();



		}

		void EFIKGrounderIKUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->weight = Weight;

			m_pData->rootRotationWeight = RootRotationWeight;

			m_pData->rootRotationSpeed = RootRotationSpeed;

			m_pData->maxRootRotationAngle = MaxRootRotationAngle;

			//m_pData->pelvis = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(mPelvis.GetNodeName());



			mSolver.Update();

			mSolver.GetData()->m_rootBone = m_pData->pelvis;



			//mLegs.Update();



		}

		void EFIKGrounderIKUI::Load() {

			if (m_pData) {

				mSolver.SetData(m_pData->solver);

				mLegs.SetData(&m_pData->legs);

			}

			else {

				mSolver.SetData(nullptr);

				mLegs.SetData(nullptr);

			}

			/*if (!m_pData) {

				return;

			}

			Weight = m_pData->weight;

			RootRotationWeight = m_pData->rootRotationWeight;

			RootRotationSpeed = m_pData->rootRotationSpeed;

			MaxRootRotationAngle = m_pData->maxRootRotationAngle;



			mPelvis.SetNode(m_pData->pelvis);

			mSolver.SetData(m_pData->solver);

			mLegs.SetData(&m_pData->legs);



			UseComponent(false);*/

		}



		UINT32 EFIKGrounderIKUI::JudgeCanUseComponent() {

			bool IsCanUse = true;

			IsCanUse = IsCanUse && mPelvis.GetNode() != nullptr;



			if (!IsCanUse) {

				return UIErrorType_ParametersAreNotFullySet;

			}



			for (int i = 0; i != mLegs.size(); ++i) {

				auto pLimb = mLegs.At(i);

				if (pLimb) {

					IsCanUse = IsCanUse && pLimb->JudgeCanUseComponent() == UIErrorType_NoError;

				}

				else {

					return UIErrorType_ParametersAreNotFullySet;

				}

			}



			return IsCanUse ? UIErrorType_NoError : UIErrorType_ChildrenParametersAreNotFullySet;

		}



		void EFIKGrounderIKUI::DefaultInitialize() {





			Weight = 1.0f;

			RootRotationWeight = 1.0f;

			RootRotationSpeed = 5.0f;

			MaxRootRotationAngle = 45.0f;

			mSolver.DefaultInitialize();



			RoleIKComponent* pRoleIKComponent = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent();



			mPelvis.SetNode(pRoleIKComponent->getBoneNodeByType(Quadruped_Bone_Pelvis));



		}





		void EFIKGrounderIKUI::PreResetComponent(bool IsUse) {

			if (IsUse) {

				for (int i = 0; i != mLegs.size(); ++i) {

					auto pLimb = mLegs.At(i);

					if (pLimb) {

						pLimb->CallUseComponent(IsUse);

					}

				}

			}

		}

		void EFIKGrounderIKUI::PostResetComponent(bool IsUse) {

			if (!IsUse) {

				for (int i = 0; i != mLegs.size(); ++i) {

					auto pLimb = mLegs.At(i);

					if (pLimb) {

						pLimb->CallUseComponent(IsUse);

					}

				}

			}

		}



	};



	//EFIKGrounderFBBIKUI

	namespace EditorFinalIK {

		EFIKGrounderFBBIKUI::EFIKGrounderFBBIKUI() {

			Init();

		}

		/*EFIKGrounderFBBIKUI::EFIKGrounderFBBIKUI(RoleObject * pRoleObject)

			: EFIKComponentBase(pRoleObject)

		{

			SetLabel("GrounderFBBIK##" + pRoleObject->getName().toString());

			Init();

			Load();

		}*/

		EFIKGrounderFBBIKUI::~EFIKGrounderFBBIKUI()

		{

		}

		void EFIKGrounderFBBIKUI::Init()

		{

			mSolver.SetLabel("Solver");

			mSolver.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



			mSpineList.SetLabel("Spine");

			mSpineList.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



			ik.SetLabel("IK");

			ik.SetShowType(UIShowType::None);

		}

		void EFIKGrounderFBBIKUI::PureShow()

		{



			IKSliderFloat("Weight", &Weight, 0.0f, 1.0f);



			ik.ShowGui();



			mSolver.ShowGui();



			IKDragFloat("Spine Bend", &SpineBend, 0.01f, FLT_MIN, FLT_MAX);

			IKDragFloat("Spine Speed", &SpineSpeed, 0.01f, 0.0f, FLT_MAX);



			mSpineList.ShowGui();



		}

		void EFIKGrounderFBBIKUI::Update()

		{

			if (!m_pData) {

				return;

			}

			m_pData->weight = Weight;

			m_pData->spineBend = SpineBend;

			m_pData->spineSpeed = SpineSpeed;

			mSolver.Update();

			ik.GetData()->ToUpdateAllFBBIK();

			

			//mSpineList.Update();

			//m_pData->ik = ik.GetFBBIK();

			//mSolver.GetData()->m_rootBone = m_pData->ik->references->pelvis;

		}

		void EFIKGrounderFBBIKUI::Load()

		{

			if (m_pData) {

				mSolver.SetData(m_pData->solver);

				mSpineList.SetData(&m_pData->spine);

			}

			else {

				mSolver.SetData(nullptr);

				mSpineList.SetData(nullptr);

			}



			/*if (!m_pData) {

				return;

			}

			Weight = m_pData->weight;

			SpineBend = m_pData->spineBend;

			SpineSpeed = m_pData->spineSpeed;



			mSolver.SetData(m_pData->solver);

			mSpineList.SetData(&m_pData->spine);

			if (m_pData->ik) {

				ik.SetData(EditorFinalIKUI::instance()->GetCurUI()->GetFBBIKManager()->Find(m_pData->ik->GetIKInfoName()));

			}

			else {

				ik.SetData(nullptr);

			}

			

			

			UseComponent(false);*/

		}



		UINT32 EFIKGrounderFBBIKUI::JudgeCanUseComponent() {

			if (ik.GetData()) {

				if (ik.GetData()->JudgeCanUseComponent() == UIErrorType_NoError) {

					return UIErrorType_NoError;

				}

				return UIErrorType_ChildrenParametersAreNotFullySet;

			}

			return UIErrorType_ParametersAreNotFullySet;

		}

		void EFIKGrounderFBBIKUI::DefaultInitialize() {



			Weight = 1.0f;



			mSolver.DefaultInitialize();

			ik.SetData(nullptr);

			SpineBend = 2.0f;

			SpineSpeed = 3.0f;

			{

				EFIKSpineEffectorUI* pEffector = new EFIKSpineEffectorUI();

				pEffector->effectorType = EchoIK::FullBodyBipedEffector::LeftShoulder;

				pEffector->horizontalWeight = 1.0f;

				pEffector->verticalWeight = 0.3f;

				mSpineList.push_back(pEffector);

			}

			{

				EFIKSpineEffectorUI* pEffector = new EFIKSpineEffectorUI();

				pEffector->effectorType = EchoIK::FullBodyBipedEffector::RightShoulder;

				pEffector->horizontalWeight = 1.0f;

				pEffector->verticalWeight = 0.3f;

				mSpineList.push_back(pEffector);

			}

			{

				EFIKSpineEffectorUI* pEffector = new EFIKSpineEffectorUI();

				pEffector->effectorType = EchoIK::FullBodyBipedEffector::Body;

				pEffector->horizontalWeight = -0.3f;

				pEffector->verticalWeight = 0.3f;

				mSpineList.push_back(pEffector);

			}

			{

				EFIKSpineEffectorUI* pEffector = new EFIKSpineEffectorUI();

				pEffector->effectorType = EchoIK::FullBodyBipedEffector::LeftHand;

				pEffector->horizontalWeight = 0.5f;

				pEffector->verticalWeight = 0.f;

				mSpineList.push_back(pEffector);

			}

			{

				EFIKSpineEffectorUI* pEffector = new EFIKSpineEffectorUI();

				pEffector->effectorType = EchoIK::FullBodyBipedEffector::RightHand;

				pEffector->horizontalWeight = 0.5f;

				pEffector->verticalWeight = 0.f;

				mSpineList.push_back(pEffector);

			}

		}



		void EFIKGrounderFBBIKUI::PreResetComponent(bool IsUse) {

			if (IsUse) {

				if (ik.GetData()) {

					ik.GetData()->CallUseComponent(IsUse);

				}

			}

		}

		void EFIKGrounderFBBIKUI::PostResetComponent(bool IsUse) {

			if (!IsUse) {

				if (ik.GetData()) {

					ik.GetData()->CallUseComponent(IsUse);

				}

			}

			else {

				if (ik.GetData()) {

					ik.GetData()->Update();

				}

			}

		}

	};



	//EFIKTargetNodeUI

	namespace EditorFinalIK {



		EFIKTargetNodeUI::EFIKTargetNodeUI() { Init(); }

		EFIKTargetNodeUI::~EFIKTargetNodeUI() {}



		void EFIKTargetNodeUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			Target.SetShowType(UIShowType::None);

			Target.SetLabel("Target");

			Locator.SetShowType(UIShowType::None);

			Locator.SetLabel("dstLocator");

		}

		void EFIKTargetNodeUI::Show() {

			Target.ShowGui();

			Locator.ShowGui();

		}

		void EFIKTargetNodeUI::Update() {



			if (LastTargetNode != Target.GetData()) {

				LastTargetNode = Target.GetData();

				EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->setFbbTargetNode(ikType, (Echo::WorldNode*)LastTargetNode);

			}

			if (LastLocator != Locator.GetDataName()) {

				LastLocator = Locator.GetDataName();

				if (!EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->m_pTargetFbbIK) {

					EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->createTargetFbb();

				}

				EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->m_pTargetFbbIK->setFbbLocatorName((EchoIK::FullBodyBipedEffector)ikType, LastLocator);

			}



		}

		void EFIKTargetNodeUI::Load() {



			LastTargetNode = nullptr;

			LastLocator = Echo::Name::g_cEmptyName;



		}



		void EFIKTargetNodeUI::DefaultInitialize() {



			Target.SetData(nullptr);

			Locator.SetData(nullptr);

			LastTargetNode = nullptr;

			LastLocator = Echo::Name::g_cEmptyName;

		}

	};



	//EFIKTargetFBBIKUI

	namespace EditorFinalIK {



		EFIKTargetFBBIKUI::EFIKTargetFBBIKUI() { Init(); }

		EFIKTargetFBBIKUI::~EFIKTargetFBBIKUI() { 

			ik.SetLabel("IK");

			ik.SetShowType(UIShowType::None);

		}

		void EFIKTargetFBBIKUI::Init() { 

			ik.SetLabel("IK");

			ik.SetShowType(UIShowType::None);



			Body.ikType = Echo::IKEffectorType::IK_Body;

			LeftHand.ikType = Echo::IKEffectorType::IK_LeftHand;

			LeftShoulder.ikType = Echo::IKEffectorType::IK_LeftShoulder;

			RightHand.ikType = Echo::IKEffectorType::IK_RightHand;

			RightShoulder.ikType = Echo::IKEffectorType::IK_RightShoulder;

			LeftFoot.ikType = Echo::IKEffectorType::IK_LeftFoot;

			LeftThigh.ikType = Echo::IKEffectorType::IK_LeftThigh;

			RightFoot.ikType = Echo::IKEffectorType::IK_RightFoot;

			RightThigh.ikType = Echo::IKEffectorType::IK_RightThigh;



			Body.SetLabel("Body");

			LeftHand.SetLabel("Left Hand");

			LeftShoulder.SetLabel("Left Shoulder");

			RightHand.SetLabel("Right Hand");

			RightShoulder.SetLabel("Right Shoulder");

			LeftFoot.SetLabel("Left Foot");

			LeftThigh.SetLabel("Left Thigh");

			RightFoot.SetLabel("Right Foot");

			RightThigh.SetLabel("Right Thigh");



			Body.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftHand.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftShoulder.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightHand.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightShoulder.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftFoot.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			LeftThigh.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightFoot.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			RightThigh.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

		}

		void EFIKTargetFBBIKUI::PureShow() {

			ik.ShowGui();



			Body.ShowGui();			 

			LeftHand.ShowGui();		 

			LeftShoulder.ShowGui();	 

			RightHand.ShowGui();	 

			RightShoulder.ShowGui(); 

			LeftFoot.ShowGui();		 

			LeftThigh.ShowGui();	 

			RightFoot.ShowGui();	 

			RightThigh.ShowGui();	 

		}

		void EFIKTargetFBBIKUI::Update() { 

		

			Body.Update();

			LeftHand.Update();

			LeftShoulder.Update();

			RightHand.Update();

			RightShoulder.Update();

			LeftFoot.Update();

			LeftThigh.Update();

			RightFoot.Update();

			RightThigh.Update();



			ik.GetData()->ToUpdateAllFBBIK();

		

		}

		void EFIKTargetFBBIKUI::Load() { 



			Body.Load();

			LeftHand.Load();

			LeftShoulder.Load();

			RightHand.Load();

			RightShoulder.Load();

			LeftFoot.Load();

			LeftThigh.Load();

			RightFoot.Load();

			RightThigh.Load();

		}



		UINT32 EFIKTargetFBBIKUI::JudgeCanUseComponent() { 

			if (ik.GetData()) {

				if (ik.GetData()->JudgeCanUseComponent() == UIErrorType_NoError) {

					return UIErrorType_NoError;

				}

				return UIErrorType_ChildrenParametersAreNotFullySet;

			}

			return UIErrorType_ParametersAreNotFullySet;

		}

		void EFIKTargetFBBIKUI::DefaultInitialize(){ 

			ik.SetData(nullptr);

			Body.DefaultInitialize();

			LeftHand.DefaultInitialize();

			LeftShoulder.DefaultInitialize();

			RightHand.DefaultInitialize();

			RightShoulder.DefaultInitialize();

			LeftFoot.DefaultInitialize();

			LeftThigh.DefaultInitialize();

			RightFoot.DefaultInitialize();

			RightThigh.DefaultInitialize();

		}



		void EFIKTargetFBBIKUI::PreResetComponent(bool IsUse) {

			if (IsUse) {

				if (ik.GetData()) {

					ik.GetData()->CallUseComponent(IsUse);

				}

			}

		}

		void EFIKTargetFBBIKUI::PostResetComponent(bool IsUse) {

			if (!IsUse) {

				if (ik.GetData()) {

					ik.GetData()->CallUseComponent(IsUse);

				}

			}

		}



	};



	//AimIK

	namespace EditorFinalIK {



		EFIKSolverBoneUI::EFIKSolverBoneUI() {}

		EFIKSolverBoneUI::~EFIKSolverBoneUI() {}

		void EFIKSolverBoneUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->transform = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(transform.GetNodeName());

			m_pData->weight = weight;

		}

		void EFIKSolverBoneUI::Load() {

			/*if (!m_pData) {

				return;

			}

			transform.SetNode(m_pData->transform);

			weight = m_pData->weight;*/

		}

		void EFIKSolverBoneUI::SetData(EchoIK::IKSolver::Bone* pData) {

			m_pData = pData;

		}





		EFIKSolverBoneGroupUI::EFIKSolverBoneGroupUI() { elementType = UIShowType::None; }

		EFIKSolverBoneGroupUI::~EFIKSolverBoneGroupUI() {}



		void EFIKSolverBoneGroupUI::push_back(Bone* pBone, float weight) {

			if (pBone == nullptr) return;

			++mCount;

			Reset();

			//mGroup[mCount - 1]->transform.SetNode(pBone);

			mGroup[mCount - 1]->weight = weight;

		}

		/*void EFIKSolverBoneGroupUI::Load() {

			if (!m_pData) {

				return;

			}

			Resize((int)m_pData->size());

			for (int i = 0; i != mCount; ++i) {

				mGroup[i]->SetData(m_pData->at(i));

			}

		}

		void EFIKSolverBoneGroupUI::Update() {

			if (!m_pData) {

				return;

			}

			int n = (int)m_pData->size();

			if (n > mCount) {

				for (int i = mCount; i != n; ++i) {

					_ECHO_DELETE((*m_pData)[i]);

				}

				m_pData->resize(mCount);

			}

			else if (n < mCount) {

				m_pData->resize(mCount);

				for (int i = n; i != mCount; ++i) {

					EchoIK::IKSolver::Bone* point = new EchoIK::IKSolver::Bone();

					(*m_pData)[i] = point;

					mGroup[i]->SetData(point);

					

				}

			}

			for (int i = 0; i != mCount; ++i) {

				mGroup[i]->Update();

			}

		}*/





		EFIKAimIKUI::EFIKAimIKUI() {

			Init();

		}

		/*EFIKAimIKUI::EFIKAimIKUI(RoleObject* pRoleObject)

			: EFIKComponentBase(pRoleObject) 

		{

			SetLabel("AimIK##" + pRoleObject->getName().toString());

			Init();

			Load();

		}*/

		EFIKAimIKUI::~EFIKAimIKUI() {}



		void EFIKAimIKUI::Init() {

		

			//target.SetLabel("Target");

			poleTarget.SetLabel("Pole Target");

			//aimTransform.SetLabel("Aim Transform");

			bones.SetLabel("Bones");

			//targetNode.SetLabel("TargetNode");//测试用

			//aimNode.SetLabel("AimNode");//测试用

			targetWorldNode.SetLabel("TargetWorldNode");

			aimTransformWorldNode.SetLabel("AimTransformWorldNode");



			//target.SetShowType(UIShowType::None);

			poleTarget.SetShowType(UIShowType::None);

			//aimTransform.SetShowType(UIShowType::None);

			bones.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			//targetNode.SetShowType(UIShowType::None);

			//aimNode.SetShowType(UIShowType::None);

			targetWorldNode.SetShowType(UIShowType::None);

			aimTransformWorldNode.SetShowType(UIShowType::None);



		}

		void EFIKAimIKUI::PureShow() {

			



			ImGui::Checkbox("Fix Transforms", &fixTransforms);

			

			targetWorldNode.ShowGui();



			//targetNode.ShowGui();

			//aimNode.ShowGui();

			//targetWorldNode.ShowGui();

			//target.ShowGui();

			if (!XY) poleTarget.ShowGui();

			//aimTransform.ShowGui();

			aimTransformWorldNode.ShowGui();



			ImGui::InputFloat3("Axis", axis.ptr());

			if (!XY) ImGui::InputFloat3("Pole Axis", poleAxis.ptr());

			

			IKSliderFloat("Weight", &weight, 0.0f, 1.0f);

			if (!XY) IKSliderFloat("Pole Weight", &poleWeight, 0.0f, 1.0f);

			IKSliderFloat("Tolerance", &tolerance, 0.0f, 1.0f);



			IKDragInt("Max Iterations", &maxIterations, 1, 0,INT_MAX);



			IKSliderFloat("Clamp Weight", &clampWeight, 0.0f, 1.0f);

			IKDragInt("Clamp Smoothing", &clampSmoothing, 0.1f, 0, 2);



			ImGui::Checkbox("Use Rotation Limits", &useRotationLimits);

			ImGui::Checkbox("2D", &XY);



			bones.ShowGui();



		

		}

		void EFIKAimIKUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->fixTransforms = fixTransforms;



			m_pData->m_pWorldTargetNode = targetWorldNode.GetData();

			m_pData->m_pWorldAimNode = aimTransformWorldNode.GetData();

			



			auto pData = m_pData->GetIKSolver();



			pData->poleTarget = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(poleTarget.GetNodeName());

			pData->axis = axis;

			pData->poleAxis = poleAxis;

			pData->IKPositionWeight = weight;

			pData->poleWeight = poleWeight;

			pData->tolerance = tolerance;

			pData->maxIterations = maxIterations;

			pData->clampWeight = clampWeight;

			pData->clampSmoothing = clampSmoothing;

			pData->useRotationLimits = useRotationLimits;

			pData->XY = XY;



			bones.Update();

		

		}

		void EFIKAimIKUI::Load() {



			if (m_pData) {

				bones.SetData(&m_pData->GetIKSolver()->bones);

			}

			else {

				bones.SetData(nullptr);

			}



			/*if (!m_pData) {

				return;

			}

			fixTransforms = m_pData->fixTransforms;





			auto pData = m_pData->GetIKSolver();

			poleTarget.SetNode(pData->poleTarget);

			axis = pData->axis;

			poleAxis = pData->poleAxis;

			weight = pData->IKPositionWeight;

			poleWeight = pData->poleWeight;

			tolerance = pData->tolerance;

			maxIterations = pData->maxIterations;

			clampWeight = pData->clampWeight;

			clampSmoothing = pData->clampSmoothing;

			useRotationLimits = pData->useRotationLimits;

			XY = pData->XY;

			bones.SetData(&pData->bones);



			UseComponent(false);*/

		}



		UINT32 EFIKAimIKUI::JudgeCanUseComponent() {

			if (!targetWorldNode.GetData() || !aimTransformWorldNode.GetData() || bones.size() < 1) {

				return UIErrorType_ParametersAreNotFullySet;

			}

			return UIErrorType_NoError;

		}

		void EFIKAimIKUI::DefaultInitialize() {



			fixTransforms = false;



			targetWorldNode.SetData(nullptr);

			aimTransformWorldNode.SetData(nullptr);





			poleTarget.SetNode(nullptr);

			axis = { 0,0,1 };

			poleAxis = { 0,1,0 };

			weight = 1.0f;//0-1

			poleWeight = 0.0f;//0-1

			tolerance = 0.001f;//0-1

			maxIterations = 4;//非负整数



			clampWeight = 0.5f;//0-1

			clampSmoothing = 2;//0-2



			useRotationLimits = true;

			XY = false;



			RoleIKComponent* pRoleIKComponent = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent();



			bones.push_back((Bone*)pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine1), 0.773f);

			bones.push_back((Bone*)pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine2), 0.883f);

			bones.push_back((Bone*)pRoleIKComponent->getBoneNodeByType(Biped_Bone_Neck), 0.93f);

			bones.push_back((Bone*)pRoleIKComponent->getBoneNodeByType(Biped_Bone_RUpperArm), 1.f);

			bones.push_back((Bone*)pRoleIKComponent->getBoneNodeByType(Biped_Bone_RHand), 1.f);

		}

	}



	//FBBIKHeadEffector

	namespace EditorFinalIK {



		EFIKFBBIKHeadEffectorBendBoneUI::EFIKFBBIKHeadEffectorBendBoneUI() {}

		EFIKFBBIKHeadEffectorBendBoneUI::~EFIKFBBIKHeadEffectorBendBoneUI() {}

		void EFIKFBBIKHeadEffectorBendBoneUI::Update() {

			if (!m_pData) {

				return;

			}

			m_pData->transform = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent()->getBoneNodeByName(transform.GetNodeName());

			m_pData->weight = weight;

		}

		void EFIKFBBIKHeadEffectorBendBoneUI::Load() {

			/*if (!m_pData) {

				return;

			}

			transform.SetNode(m_pData->transform);

			weight = m_pData->weight;*/

		}







		EFIKFHEBendBoneGroupUI::EFIKFHEBendBoneGroupUI() { elementType = UIShowType::None; }

		EFIKFHEBendBoneGroupUI::~EFIKFHEBendBoneGroupUI() {}



		void EFIKFHEBendBoneGroupUI::push_back(WorldNode* pBone, float weight) {

			if (pBone == nullptr) return;

			++mCount;

			Reset();

			mGroup[mCount - 1]->transform.SetNode(pBone);

			mGroup[mCount - 1]->weight = weight;

		}

		/*void EFIKFHEBendBoneGroupUI::Load() {

			if (!m_pData) {

				return;

			}

			Resize((int)m_pData->size());

			for (int i = 0; i != mCount; ++i) {

				mGroup[i]->SetData(m_pData->at(i));

			}

		}

		void EFIKFHEBendBoneGroupUI::Update() {

			if (!m_pData) {

				return;

			}

			int n = (int)m_pData->size();

			if (n > mCount) {

				for (int i = mCount; i != n; ++i) {

					_ECHO_DELETE((*m_pData)[i]);

				}

				m_pData->resize(mCount);

			}

			else if (n < mCount) {

				m_pData->resize(mCount);

				for (int i = n; i != mCount; ++i) {

					EchoIK::FBBIKHeadEffector::BendBone* bone = new EchoIK::FBBIKHeadEffector::BendBone();

					(*m_pData)[i] = bone;

					mGroup[i]->SetData(bone);

				}

			}

			for (int i = 0; i != mCount; ++i) {

				mGroup[i]->Update();

			}

		}*/





		EFIKFBBIKHeadEffectorUI::EFIKFBBIKHeadEffectorUI() { Init();}

		/*EFIKFBBIKHeadEffectorUI::EFIKFBBIKHeadEffectorUI(RoleObject* pRoleObject) 

			: EFIKComponentBase(pRoleObject)

		{

			SetLabel("FBBIKHeadEffector##" + pRoleObject->getName().toString());

			Init();

			Load();

		}*/

		EFIKFBBIKHeadEffectorUI::~EFIKFBBIKHeadEffectorUI() {}



		void EFIKFBBIKHeadEffectorUI::Init() {

			ik.SetLabel("IK");

			bendBones.SetLabel("Bend Bones");

			CCDBones.SetLabel("CCD Bones");

			stretchBones.SetLabel("Stretch Bones");

			chestBones.SetLabel("Chest Bones");



			ik.SetShowType(UIShowType::None);

			bendBones.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			CCDBones.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			stretchBones.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

			chestBones.SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);



			Target.SetLabel("Target");

			Target.SetShowType(UIShowType::None);

		}

		void EFIKFBBIKHeadEffectorUI::PureShow() {

		 

			Target.ShowGui();

			ik.ShowGui();

			ImGui::NewLine();

			ImGui::Text("Position");

			IKSliderFloat("Position Weight", &positionWeight);

			IKSliderFloat("Body Weight", &bodyWeight);

			IKSliderFloat("Thigh Weight", &thighWeight);

			ImGui::Checkbox("Hands Pull Body", &handsPullBody);

			ImGui::NewLine();

			ImGui::Text("Rotation");

			IKSliderFloat("Rotation Weight", &rotationWeight);

			IKSliderFloat("Body Clamp Weight", &bodyClampWeight);

			IKSliderFloat("Head Clamp Weight", &headClampWeight);

			IKSliderFloat("Bend Weight", &bendWeight);

			bendBones.ShowGui();

			ImGui::NewLine();

			ImGui::Text("CCD");

			IKSliderFloat("CCD Weight", &CCDWeight);

			IKSliderFloat("Roll", &roll);

			IKDragFloat("Damper", &damper, 1, 0, 1000);

			CCDBones.ShowGui();

			ImGui::NewLine();

			ImGui::Text("Stretching");

			IKSliderFloat("Post Stretch Weight", &postStretchWeight);

			IKDragFloat("Max Stretch", &maxStretch, 0.01f, FLT_MIN, FLT_MAX);

			IKDragFloat("Stretch Damper", &stretchDamper, 0.01f, 0.0f, FLT_MAX);

			ImGui::Checkbox("Fix Head", &fixHead);

			stretchBones.ShowGui();

			ImGui::NewLine();

			ImGui::Text("Chest Direction");

			ImGui::InputFloat3("Chest Direction", chestDirection.ptr());

			IKSliderFloat("Chest Direction Weight", &chestDirectionWeight);

			chestBones.ShowGui();

			 

		}

		void EFIKFBBIKHeadEffectorUI::Update() {

			if (!m_pData) {

				return;

			}



			if (LastTargetNode != Target.GetData()) {

				LastTargetNode = Target.GetData();

				m_pData->setWorldTargetNode((WorldNode*)LastTargetNode);

			}

			ik.GetData()->ToUpdateAllFBBIK();

			//m_pData->ik = ik.GetFBBIK();

			m_pData->positionWeight = positionWeight;

			m_pData->bodyWeight = bodyWeight;

			m_pData->thighWeight = thighWeight;

			m_pData->handsPullBody = handsPullBody;

			m_pData->rotationWeight = rotationWeight;

			m_pData->bodyClampWeight = bodyClampWeight;

			m_pData->headClampWeight = headClampWeight;

			m_pData->bendWeight = bendWeight;

			//bendBones.Update();

			m_pData->CCDWeight = CCDWeight;

			m_pData->roll = roll;

			m_pData->damper = damper;

			//CCDBones.Update();

			m_pData->postStretchWeight = postStretchWeight;

			m_pData->maxStretch = maxStretch;

			m_pData->stretchDamper = stretchDamper;

			m_pData->fixHead = fixHead;

			//stretchBones.Update();

			m_pData->chestDirection = chestDirection;

			m_pData->chestDirectionWeight = chestDirectionWeight;

			//chestBones.Update();

		}

		void EFIKFBBIKHeadEffectorUI::Load() {

			if (m_pData) {

				bendBones.SetData(&m_pData->bendBones);

				CCDBones.SetData(&m_pData->CCDBones);

				stretchBones.SetData(&m_pData->stretchBones);

				chestBones.SetData(&m_pData->chestBones);

			}

			else {

				bendBones.SetData(nullptr);

				CCDBones.SetData(nullptr);

				stretchBones.SetData(nullptr);

				chestBones.SetData(nullptr);

			}

			LastTargetNode = nullptr;

			/*if (!m_pData) {

				return;

			}

			if (m_pData->ik) {

				ik.SetData(EditorFinalIKUI::instance()->GetCurUI()->GetFBBIKManager()->Find(m_pData->ik->GetIKInfoName()));

			}

			else {

				ik.SetData(nullptr);

			}

			positionWeight = m_pData->positionWeight;

			bodyWeight = m_pData->bodyWeight;

			thighWeight = m_pData->thighWeight;

			handsPullBody = m_pData->handsPullBody;

			rotationWeight = m_pData->rotationWeight;

			bodyClampWeight = m_pData->bodyClampWeight;

			headClampWeight = m_pData->headClampWeight;

			bendWeight = m_pData->bendWeight;

			bendBones.SetData(&m_pData->bendBones);

			CCDWeight = m_pData->CCDWeight;

			roll = m_pData->roll;

			damper = m_pData->damper;

			CCDBones.SetData(&m_pData->CCDBones);

			postStretchWeight = m_pData->postStretchWeight;

			maxStretch = m_pData->maxStretch;

			stretchDamper = m_pData->stretchDamper;

			fixHead = m_pData->fixHead;

			stretchBones.SetData(&m_pData->stretchBones);

			chestDirection = m_pData->chestDirection;

			chestDirectionWeight = m_pData->chestDirectionWeight;

			chestBones.SetData(&m_pData->chestBones);



			UseComponent(false);*/

		}



		UINT32 EFIKFBBIKHeadEffectorUI::JudgeCanUseComponent() {

			if (ik.GetData()) {

				if (ik.GetData()->JudgeCanUseComponent() == UIErrorType_NoError) {

					return UIErrorType_NoError;

				}

				return UIErrorType_ChildrenParametersAreNotFullySet;

			}

			return UIErrorType_ParametersAreNotFullySet;

		}

		void EFIKFBBIKHeadEffectorUI::DefaultInitialize() {



			positionWeight = 1.0f;

			bodyWeight = 0.8f;

			thighWeight = 0.0f;

			handsPullBody = true;

			rotationWeight = 0.3f;

			bodyClampWeight = 0.5f;

			headClampWeight = 0.5f;

			bendWeight = 0.6f;

			CCDWeight = 0.0f;

			roll = 0.0f;

			damper = 300.0f;

			postStretchWeight = 1.0f;

			maxStretch = 0.1f;

			stretchDamper = 0.0f;

			fixHead = false;

			chestDirection = { 0,0,1 };

			chestDirectionWeight = 1.0f;





			RoleIKComponent* pRoleIKComponent = EditorFinalIKUI::instance()->GetCurUI()->GetRoleObject()->getRoleIKComponent();



			CCDBones.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine1));

			CCDBones.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine2));

			CCDBones.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Neck));



			bendBones.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine1), 1.0f);

			bendBones.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Spine2), 1.0f);

			bendBones.push_back(pRoleIKComponent->getBoneNodeByType(Biped_Bone_Neck), 1.0f);



		}





		void EFIKFBBIKHeadEffectorUI::PreResetComponent(bool IsUse) { 

			if (IsUse) {

				ik.GetData()->CallUseComponent(IsUse);

			}

		}

		void EFIKFBBIKHeadEffectorUI::PostResetComponent(bool IsUse) {

			if (!IsUse) {

				ik.GetData()->CallUseComponent(IsUse);

			}

			else {

				if (ik.GetData()) {

					ik.GetData()->Update();

				}

			}

		}

	}



	//EchoNodeUI

	namespace EditorFinalIK {

		/*EditorEchoNodeUI::EditorEchoNodeUI() { 

			m_pData = new WorldNode(Name("TargetNode"), Vector3::ZERO, Quaternion::IDENTITY, Vector3::UNIT_SCALE);

			Init();

			Load();

		}*/

		/*EditorEchoNodeUI::EditorEchoNodeUI(RoleObject* pRoleObject)

		{

			m_pData = new WorldNode(Name("TargetNode"),Vector3::ZERO,Quaternion::IDENTITY,Vector3::UNIT_SCALE);

			SetLabel("TargetNode##" + pRoleObject->getName().toString());

			Init();

			Load();

		}*/

		/*EditorEchoNodeUI::~EditorEchoNodeUI() { _ECHO_DELETE(m_pData); }

		void EditorEchoNodeUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

		}

		void EditorEchoNodeUI::Show() {

			

			ImGui::DragFloat3(GetLabel(), position.ptr(),0.01f,-1000.0f,1000.0f);

			Update();

		}

		void EditorEchoNodeUI::Update() {

			m_pData->_setDerivedPosition(position);

		}

		void EditorEchoNodeUI::Load() {

			position = m_pData->_getDerivedPosition();

		}*/







		EchoWorldNodeUI::EchoWorldNodeUI() {

			Init();

		}

		EchoWorldNodeUI::~EchoWorldNodeUI() {  }

		void EchoWorldNodeUI::Init() {

			SetShowType(UIShowType::TreeNodeEx_OpenOnArrow);

		}

		void EchoWorldNodeUI::Show() {

			if (m_pData) {

				ImGui::InputFloat3("Position", (float*)m_pData->getPosition().ptr(),"%.3f", ImGuiInputTextFlags_ReadOnly);

				ImGui::InputFloat4("Quaternion", (float*)m_pData->getOrientation().ptr() , "%.3f", ImGuiInputTextFlags_ReadOnly);

				ImGui::InputFloat3("Scale", (float*)m_pData->getScale().ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

			}

		}

		void EchoWorldNodeUI::Update() { }

		void EchoWorldNodeUI::Load() { }







		WorldNodeSelectUI::WorldNodeSelectUI() { 

			PopupLabel = std::string("SelectWorldNode##") + PtrToString(this);

		}

		WorldNodeSelectUI::~WorldNodeSelectUI() {}

		void WorldNodeSelectUI::OnButton2(bool IsClick) {



			const std::map<Name, WorldNode*>& mp = Root::instance()->getWorldNode();

			WorldNode* selItem = GetData();

			if (OnButton2UseMap(IsClick, &mp, PopupLabel, &selItem)) {

				SetData(selItem);

			}



		}

		void WorldNodeSelectUI::MessageResponse() { }

		void WorldNodeSelectUI::OnButton1(bool IsClick) {

			if (IsClick && m_pData) {

				ImGui::OpenPopup((std::string("ShowWorldNode##") + PtrToString(this)).c_str());

			}

			//ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() * 0.5f));

			if (ImGui::BeginPopup((std::string("ShowWorldNode##") + PtrToString(this)).c_str())) {

				worldNodeUI.ShowGui(UIShowType::None, 0.9f);

				ImGui::EndPopup();

			}

		}

		void WorldNodeSelectUI::SetData(Echo::WorldNode* pData) {

			m_pData = pData;

			worldNodeUI.SetData(pData);

			if (m_pData) {

				SetSelectName(pData->getName().c_str());

			}

			else {

				SetSelectName("None");

			}

		

		}



		const Name& WorldNodeSelectUI::GetDataName() {

			if (m_pData) {

				return m_pData->getName();

			}

			else {

				return NullName;

			}

		}



		const ImVec4& WorldNodeSelectUI::ButtonColor() {

			if (m_pData) {

				return DefaultButtonColor;

			}

			return WorldNodeSelectButtonColor;

		}

	}



	//LocatorSelectUI

	namespace EditorFinalIK {

	

		LocatorSelectUI::LocatorSelectUI() {

			PopupLabel = std::string("SelectLocator##") + PtrToString(this);

			m_pData = new Name();

		}

		LocatorSelectUI::~LocatorSelectUI() { _ECHO_DELETE(m_pData); }



		void LocatorSelectUI::OnButton2(bool IsClick) {

		

			RolePtr roleInfo = CURRENTALLUI->GetRoleObject()->getSkeletonComponent()->getRoleInfo();

			LocatorStructMap locatorMap;

			std::vector<Name> locatorNames;

			roleInfo->getAllLocatorInfo(locatorMap);

			locatorNames.reserve(locatorMap.size());

			for (auto locatorIter : locatorMap) {

				locatorNames.emplace_back(locatorIter.first);

			}

			Name selName = GetDataName();

			if (OnButton2UseVector(IsClick, locatorNames, PopupLabel, selName)) {

				SetData(&selName);

			}



		}

		void LocatorSelectUI::MessageResponse() {}

		void LocatorSelectUI::OnButton1(bool IsClick) {}

		void LocatorSelectUI::SetData(Echo::Name* pData) {

			if (!pData) {

				return;

			}

			*m_pData = *pData;

			if (pData->empty()) {

				SetSelectName("None");

			}

			else {

				SetSelectName(pData->c_str());

			}

		}



		const Name& LocatorSelectUI::GetDataName() {

			if (m_pData) {

				return *m_pData;

			}

			else {

				return NullName;

			}

		}



		const ImVec4& LocatorSelectUI::ButtonColor() {

			if (m_pData) {

				return DefaultButtonColor;

			}

			return LocatorSelectButtonColor;

		}

	

	}



	//ManagerUI

	namespace EditorFinalIK {

		

		//EFIKLimbManagerUI-------------------------------------------------------

		EFIKLimbManagerUI::EFIKLimbManagerUI() { 

			SetLabel("LimbIK##" + PtrToString((this)));

			mInfoType = FinalIKType_LimbIK;

		}

		EFIKLimbManagerUI::~EFIKLimbManagerUI() {}

		



		//EFIKFBBIKManagerUI-------------------------------------------------------

		EFIKFBBIKManagerUI::EFIKFBBIKManagerUI() { 

			SetLabel("FullBodyBipedIK##" + PtrToString((this))); 

			mInfoType = FinalIKType_FullBodyBipedIK;

		}

		EFIKFBBIKManagerUI::~EFIKFBBIKManagerUI() {}





		//EFIKGrounderQuadrupedManagerUI-------------------------------------------

		EFIKGrounderQuadrupedManagerUI::EFIKGrounderQuadrupedManagerUI() { 

			SetLabel("GrounderQuadruped##" + PtrToString((this)));

			mInfoType = FinalIKType_GrounderQuadruped;

		}

		EFIKGrounderQuadrupedManagerUI::~EFIKGrounderQuadrupedManagerUI() {}





		//EFIKGrounderIKManagerUI---------------------------------------------------

		EFIKGrounderIKManagerUI::EFIKGrounderIKManagerUI() { 

			SetLabel("GrounderIK##" + PtrToString((this)));

			mInfoType = FinalIKType_GrounderIK;

		}

		EFIKGrounderIKManagerUI::~EFIKGrounderIKManagerUI() {}





		//EFIKGrounderFBBIKManagerUI-------------------------------------------------

		EFIKGrounderFBBIKManagerUI::EFIKGrounderFBBIKManagerUI() { 

			SetLabel("GrounderFBBIK##" + PtrToString((this))); 

			mInfoType = FinalIKType_GrounderFBBIK;

		}

		EFIKGrounderFBBIKManagerUI::~EFIKGrounderFBBIKManagerUI() {}



		//EFIKAimIKManagerUI-------------------------------------------------

		EFIKAimIKManagerUI::EFIKAimIKManagerUI() {

			SetLabel("AimIK##" + PtrToString((this)));

			mInfoType = FinalIKType_AimIK;

		}

		EFIKAimIKManagerUI::~EFIKAimIKManagerUI() {}



		//EFIKAimIKManagerUI-------------------------------------------------

		EFIKFBBIKHeadEffectorManagerUI::EFIKFBBIKHeadEffectorManagerUI() {

			SetLabel("FBBIKHeadEffector##" + PtrToString((this)));

			mInfoType = FinalIKType_FBBIKHeadEffector;

		}

		EFIKFBBIKHeadEffectorManagerUI::~EFIKFBBIKHeadEffectorManagerUI() {}



		//EFIKTargetFBBIKManagerUI-----------------------------------------



		EFIKTargetFBBIKManagerUI::EFIKTargetFBBIKManagerUI() {

			SetLabel("TargetFBBIK##" + PtrToString((this)));

			mInfoType = FinalIKType_TargetFBBIK;

		}

		EFIKTargetFBBIKManagerUI::~EFIKTargetFBBIKManagerUI() {}



	};



	void AddNodeToMap(std::map<Name, Bone*>& boneMap, Bone* pBone) {

		if (pBone->isTagPoint()) {

			return;

		}

		boneMap.insert(std::make_pair(pBone->getName(), pBone));

		int num = pBone->numChildren();

		if (num != 0) {

			for (int i = 0; i != num; ++i) {

				AddNodeToMap(boneMap, (Echo::Bone*)(pBone->getChild(i)));

			}

		}

	}



	void AddWorldNodeToVec(std::vector<WorldNode*>& nodeVec, WorldNode* pNode) {

		if (!pNode) {

			return;

		}

		nodeVec.push_back(pNode);

		auto iter = pNode->getChildIterator();

		for (auto it = iter.begin(); it != iter.end(); ++it) {

			AddWorldNodeToVec(nodeVec, *it);

		}

	}



	WorldNode* GetWorldNodeRoot(WorldNode* pNode) {

		if (!pNode) {

			return nullptr;

		}

		WorldNode* pParent = pNode->getParent();

		if (pParent) {

			return GetWorldNodeRoot(pParent);

		}

		else {

			return pNode;

		}

	}



	//FinalIKALLUI

	namespace EditorFinalIK {



		const int FinalIKALLUI::LoadOrder[FinalIKUIEnum_Size] = { 0, 1, 2, 3, 4, 5, 6, 7 };

		const int FinalIKALLUI::SaveOrder[FinalIKUIEnum_Size] = { 0, 1, 2, 3, 4, 5, 6, 7 };

		const int FinalIKALLUI::ShowOrder[FinalIKUIEnum_Size] = { 0, 1, 2, 3, 4, 5, 6, 7 };

		const int FinalIKALLUI::DeleOrder[FinalIKUIEnum_Size] = { 2, 3, 4, 5, 6, 7, 0, 1 };



		const char* FinalIKALLUI::FinalIKUIEnumLabel(FinalIKUIEnum type) {

			static std::map<FinalIKUIEnum, const char*> LabelMap = {

				{ LimbIK,"LimbIK"},

				{ FullBodyBipedIK,"FullBodyBipedIK"},

				{ GrounderQuadruped,"GrounderQuadruped"},

				{ GrounderIK,"GrounderIK"},

				{ GrounderFBBIK,"GrounderFBBIK"},

				{ AimIK,"AimIK"},

				{ FBBIKHeadEffector,"FBBIKHeadEffector"},

				{ TargetFBBIK, "TargetFBBIK"}

			};

			return LabelMap[type];

		}



		//EFIKGrounderQuadrupedUI* FinalIKALLUI::CreateGrounderQuadrupedUI(RoleObject* pRoleObject) {

		//	EFIKGrounderQuadrupedUI* pUI = new EFIKGrounderQuadrupedUI(pRoleObject);

		//	return pUI;

		//}

		//EFIKGrounderIKUI* FinalIKALLUI::CreateGrounderIKUI(RoleObject* pRoleObject) {

		//	EFIKGrounderIKUI* pUI = new EFIKGrounderIKUI(pRoleObject);

		//	return pUI;

		//}

		//EFIKGrounderFBBIKUI * FinalIKALLUI::CreateGrounderFBBIKUI(RoleObject * pRoleObject)

		//{

		//	EFIKGrounderFBBIKUI* pUI = new EFIKGrounderFBBIKUI(pRoleObject);

		//	return pUI;

		//}

		//EFIKFBBIKManagerUI* FinalIKALLUI::CreateFBBIKManagerUI(RoleObject* pRoleObject) {

		//	EFIKFBBIKManagerUI* pUI = new EFIKFBBIKManagerUI();

		//	return pUI;

		//}

		//EFIKLimbManagerUI* FinalIKALLUI::CreateLimbManagerUI(RoleObject* pRoleObject) {

		//	EFIKLimbManagerUI* pUI = new EFIKLimbManagerUI();

		//	return pUI;

		//}



		EditorManagerBase* FinalIKALLUI::CreateUI(FinalIKUIEnum type) {

			if (type == FinalIKUIEnum_Size) {

				return nullptr;

			}

			if (mUIVector[(int)type]) {

				return mUIVector[(int)type];

			}

			EditorManagerBase* res = nullptr;

			switch (type) {

			case GrounderQuadruped: {

				res = new EFIKGrounderQuadrupedManagerUI();

				break;

			}

			case GrounderIK: {

				res = new EFIKGrounderIKManagerUI();

				break;

			}

			case GrounderFBBIK: {

				res = new EFIKGrounderFBBIKManagerUI();

				break;

			}

			case FullBodyBipedIK: {

				res = new EFIKFBBIKManagerUI();

				break;

			}

			case LimbIK: {

				res = new EFIKLimbManagerUI();

				break;

			}

			case AimIK: {

				res = new EFIKAimIKManagerUI();

				break;

			}

			case FBBIKHeadEffector: {

				res = new EFIKFBBIKHeadEffectorManagerUI();

				break;

			}

			case TargetFBBIK: {

				res = new EFIKTargetFBBIKManagerUI();

				break;

			}

			default: {

				break;

			}

			}

			mUIVector[(int)type] = res;

			return res;

		}

		bool FinalIKALLUI::AddComponent() {

			int SelIndex = -1;

			for (int i = 0; i != FinalIKUIEnum_Size; ++i) {

				if (ImGui::Selectable(FinalIKUIEnumLabel(FinalIKUIEnum(i)))) {

					SelIndex = i;

				}

			}

			if (SelIndex == -1) {

				return false;

			}

			else {

				return CreateUI(FinalIKUIEnum(SelIndex)) != nullptr;

			}

		}

		bool FinalIKALLUI::RemoveComponent(FinalIKUIEnum DelType) {

			if (DelType == FinalIKUIEnum_Size) {

				return false;

			}

			_ECHO_DELETE(mUIVector[(int)DelType]);

			return true;

		}



		FinalIKALLUI::FinalIKALLUI(RoleObject* RoleObj)

		{

			pRoleObj = RoleObj;

			

			addComponentPopupLabel = ("Add Component##" + pRoleObj->getName().toString() + PtrToString(this));

			mNamePool.clear();//mNamePool.insert(Name(""));

			mPtrPool.clear();//mPtrPool.insert(nullptr);

			mBoneMap.clear();

		}

		FinalIKALLUI::~FinalIKALLUI() {



			/*for (auto ui : mUIVector) {

				_ECHO_DELETE(ui);

			}

			mUIVector.clear();*/



			for (int i = 0; i != mUIVector.size(); ++i) {

				_ECHO_DELETE(mUIVector[DeleOrder[i]]);

			}

			mUIVector.clear();



			mNamePool.clear();

			mPtrPool.clear();

			mBoneMap.clear();

		}



		void FinalIKALLUI::InitGui() {

			

			Echo::RoleMovable* pSkeObj = pRoleObj->getRoleMovable();

			if (pSkeObj) {

				Echo::SkeletonInstance* sklIns = pSkeObj->getSkeleton();

				if (sklIns) {

					Bone *pBone = sklIns->getRootBone();

					if (pBone) {

						AddNodeToMap(mBoneMap, pBone);

					}

				}

			}

			

			mUIVector.resize((int)(FinalIKUIEnum_Size));



			LoadJSON();

			

			PushIsDisable(false);

		}

		void FinalIKALLUI::ShowGui() {

			ImVec2 size = ImGui::GetWindowSize();

			FinalIKUIEnum delType = FinalIKUIEnum_Size;

			for (int i = 0; i != mUIVector.size(); ++i) {

				if (mUIVector[ShowOrder[i]] && mUIVector[ShowOrder[i]]->ShowGui()) {

					delType = (FinalIKUIEnum)ShowOrder[i];

				}

			}

			RemoveComponent(delType);

			if (ImGui::ButtonEx("Add Component", ImVec2(size.x, 0.f))) {

				ImGui::OpenPopup(addComponentPopupLabel.c_str());

			}

			ImGui::SetNextWindowSize(ImVec2(size * 0.5f));

			if (ImGui::BeginPopup(addComponentPopupLabel.c_str())) {

				AddComponent();

				ImGui::EndPopup();

			}

		}



		void FinalIKALLUI::SaveJSON() {



			RolePtr roleInfo = pRoleObj->getSkeletonComponent()->getRoleInfo();

			if (roleInfo.isNull()) {

				return;

			}

			if (roleInfo->FinalIKisNull()) {

				roleInfo->CreateFinalIK();

			}

			roleInfo->mFinalIKInfo->mAllIKMap.clear();



			for (int i = 0; i != mUIVector.size(); ++i) {

				if (mUIVector[SaveOrder[i]] && !mUIVector[SaveOrder[i]]->Empty()) {

					mUIVector[SaveOrder[i]]->WriteInfo(roleInfo->mFinalIKInfo);

				}

			}

			/*for (std::vector<EditorManagerBase*>::iterator ui = mUIVector.begin(); ui != mUIVector.end(); ui++) {

				if ((*ui) && !(*ui)->Empty()) {

					(*ui)->WriteInfo(roleInfo->mFinalIKInfo);

				}

			}*/

			roleInfo->exportToJson();



		}

		void FinalIKALLUI::LoadJSON() {



			RolePtr roleInfo = pRoleObj->getSkeletonComponent()->getRoleInfo();

			roleInfo->loadFromJson();

			if (roleInfo.isNull() || roleInfo->FinalIKisNull()) {

				return;

			}

			//for (int i = 0; i != mUIVector.size(); ++i) {

			//	FinalIKUIEnum type = (FinalIKUIEnum)i;

			//	CreateUI(type);

			//	mUIVector[i]->ReadInfo(roleInfo->mFinalIKInfo);

			//	if (mUIVector[i]->Empty()) {

			//		RemoveComponent(type);

			//	}

			//}



			for (int i = 0; i != mUIVector.size(); ++i) {

				_ECHO_DELETE(mUIVector[DeleOrder[i]]);

			}



			for (int i = 0; i != mUIVector.size(); ++i) {



				FinalIKUIEnum type = (FinalIKUIEnum)LoadOrder[i];

				CreateUI(type);

				mUIVector[LoadOrder[i]]->ReadInfo(roleInfo->mFinalIKInfo);

				if (mUIVector[LoadOrder[i]]->Empty()) {

					RemoveComponent(type);

				}

			}

			

		}



		RoleObject* FinalIKALLUI::GetRoleObject() {

			return pRoleObj;

		}

		EFIKGrounderQuadrupedUI* FinalIKALLUI::GetGrounder() {

			EFIKGrounderQuadrupedManagerUI* p = (EFIKGrounderQuadrupedManagerUI*)mUIVector[(int)GrounderQuadruped];

			if (p && !p->Empty()) {

				return p->GetData()->begin()->second;

			}

			return nullptr;

		}

		std::map<Name, EFIKLimbIKUI*>* FinalIKALLUI::GetLimbMap() {

			EFIKLimbManagerUI* pMan = (EFIKLimbManagerUI*)mUIVector[(int)LimbIK];

			if (pMan) {

				return pMan->GetData();

			}

			else {

				return nullptr;

			}

		}

		std::map<Name, EFIKFullBodyBipedIKUI*>* FinalIKALLUI::GetFBBIKMap() {

			EFIKFBBIKManagerUI* pMan = (EFIKFBBIKManagerUI*)mUIVector[(int)FullBodyBipedIK];

			if (pMan) {

				return pMan->GetData();

			}

			else {

				return nullptr;

			}

		}

		EFIKFBBIKManagerUI* FinalIKALLUI::GetFBBIKManager() {

			EFIKFBBIKManagerUI* pMan = (EFIKFBBIKManagerUI*)mUIVector[(int)FullBodyBipedIK];

			return pMan;

		}

		std::map<Name, Bone*>* FinalIKALLUI::GetSkeletonMap() {

			return &mBoneMap;

		}

		std::vector<WorldNode*>* FinalIKALLUI::GetWorldNodeVec() {

			//return &mWorldNodeVec;

			return nullptr;

		}

		/*InfoIO& FinalIKALLUI::GetInfoIO() {

			return mInfoIO;

		}*/





		bool FinalIKALLUI::PtrIsExist(void *ptr) {

			return mPtrPool.find(ptr) != mPtrPool.end();

		}

		void FinalIKALLUI::AddPtrToPool(void *ptr) {

			if (ptr != nullptr) {

				if (PtrIsExist(ptr)) {

					return;

				}

				mPtrPool.insert(ptr);

			}

		}

		void FinalIKALLUI::RemovePtrFromPool(void*ptr) {

			auto it = mPtrPool.find(ptr);

			if (it != mPtrPool.end()) {

				mPtrPool.erase(it);

			}

		}



		bool FinalIKALLUI::NameIsExist(const Name& name) {

			return mNamePool.find(name) != mNamePool.end();

		}

		void FinalIKALLUI::AddNameToPool(const Name& name) {

			mNamePool.insert(name);

		}

		void FinalIKALLUI::RemoveNameFromPool(const Name& name) {

			mNamePool.erase(name);

		}



	};



	

	//EditorFinalIKUI

	namespace EditorFinalIK {



		bool EditorFinalIKUI::IsCreated = false;



		void EditorFinalIKUI::InitGui(int classID)

		{

			mUIType = U8("角色IK界面");

			mUIName = U8("角色IK界面##") + std::to_string(classID);

			ImGui::SetNextWindowPos(mPosition, mPositionType);

			ImGui::SetNextWindowSize(mSize, mSizeType);



			//mUIStyle.showMenu = true;

		}



		void EditorFinalIKUI::Keyboard() {}



		EditorFinalIKUI::EditorFinalIKUI(int classID, ImGuiContext * ctx)

			: BaseUI(classID, ctx)

			, mPosition(0.f, 0)

			, mPositionType(ImGuiCond_FirstUseEver)

			, mSize(300.f, 300)

			, mSizeType(ImGuiCond_FirstUseEver)

		{

			InitGui(classID);

			//new PhysXObjectMananger();

			IsCreated = true;

		}



		EditorFinalIKUI::~EditorFinalIKUI()

		{



			for (auto roleIK : mAllRoleIKUI) {

				m_pCurUI = roleIK.second;

				_ECHO_DELETE(roleIK.second);

			}

			mAllRoleIKUI.clear();

			//delete PhysXObjectMananger::instance();

			IsCreated = false;

		}

		void EditorFinalIKUI::ShowGui()

		{



			if (!ImGui::BeginChild(mUIName.c_str())) {

				ImGui::EndChild();

				return;

			}

			if (!pCurRoleObj) {

				ImGui::Text("No Selected Role");

				/*{

					static std::vector<EchoRigidBodyImpl*> Rigids;

					static Vector3 position;

					static Quaternion rotation;

					static Vector3 halfSize;

					static float mass = 1.0f;

					static bool IsUseGravity = true;

					ImGui::InputFloat3("Position", position.ptr());

					ImGui::InputFloat4("Rotation", rotation.ptr());

					ImGui::InputFloat3("HalfSize", halfSize.ptr());

					ImGui::InputFloat("Mass", &mass);

					ImGui::Checkbox("Gravity", &IsUseGravity);



					if (ImGui::Button("Add RigidBody")) {

						Rigids.push_back(PhysXObjectMananger::instance()->CreateObject(Name(std::to_string(Rigids.size())),position,rotation,halfSize,mass));

						Rigids.back()->SetGravity(IsUseGravity);

					}

					if (ImGui::Button("Import Scene Rigidbody")) {

						PhysXObjectMananger::instance()->ImportSceneRigidBody();

						std::vector<Name> names;

						PhysXObjectMananger::instance()->GetNames(names);

						for (const Name& name : names) {

							Rigids.push_back(PhysXObjectMananger::instance()->GetRigidBody(name));

						}

					}

 					int deleteIndex = -1;

					for (int i = 0; i != Rigids.size(); ++i) {

						ImGui::PushID(i);

						Vector3 pos;

						Quaternion quat;

						Rigids[i]->GetPosition(pos);

						Rigids[i]->GetRotation(quat);

						ImGui::Text("%d", i); ImGui::SameLine(); 

 						if (ImGui::Button("Delete")) { 

 							deleteIndex = i;

 						}

						ImGui::InputFloat3("RigidBody Position", pos.ptr(),"%.3f", ImGuiInputTextFlags_ReadOnly);

						ImGui::InputFloat4("RigidBody Rotation", quat.ptr(), "%.3f", ImGuiInputTextFlags_ReadOnly);

						ImGui::PopID();

					}

 					if (deleteIndex != -1) {

 						PhysXObjectMananger::instance()->RemoveObject(Name(std::to_string(deleteIndex)));

 						Rigids.erase(Rigids.begin() + deleteIndex);

 					}

				}*/

				/*{

				

					static std::vector<Echo::Name> bankPaths;

					if (ImGui::Button("读取Bank")) {

						bankPaths.clear();

						BellSystem::instance()->getBankPathList(bankPaths);

					}

					char buf[200] = { '\0' };

					ImGui::InputText("bank Name", buf, 200);

					if (ImGui::Button("加载Bank")) {

						bankPaths.clear();

					}

					ImGui::NewLine();



				}*/

				ImGui::EndChild();

				return;

			}

			if (IsLockRoleObj) {

				ImGui::PushStyleColor(ImGuiCol_Button, LockRoleButtonColor);

				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, LockRoleButtonHoveredColor);

				if (ImGui::Button(pCurRoleObj->getName().c_str(),ImVec2(ImGui::GetWindowWidth(), 0.f))) {

					IsLockRoleObj = false;

				}

				ImGui::PopStyleColor(2);

			}

			else if (ImGui::Button("Lock Role",ImVec2(ImGui::GetWindowWidth(), 0.f))) {

				IsLockRoleObj = true;

			}



			ImVec2 size = ImVec2(ImGui::GetWindowWidth()*0.5f, 0.f);

			if (ImGui::Button("Save To Json", size)) SaveComponent();

			ImGui::SameLine();

			if (ImGui::Button("Load From Json", size)) LoadComponent();



			m_pCurUI->ShowGui();



			ImGui::EndChild();



			return;

		}



		void EditorFinalIKUI::SaveComponent()

		{

			if (m_pCurUI) {

				m_pCurUI->SaveJSON();

			}

		}



		void EditorFinalIKUI::LoadComponent()

		{

			if (m_pCurUI) {

				m_pCurUI->LoadJSON();

			}

		}



		FinalIKALLUI* EditorFinalIKUI::GetCurUI() {

			return m_pCurUI;

		}



		void EditorFinalIKUI::DestroyRoleObject(RoleObject* pRoleObj) {

			if (!pRoleObj) {

				return;

			}

			auto iter = mAllRoleIKUI.find(pRoleObj);

			if (iter != mAllRoleIKUI.end()) {

				RoleObject* pCurRoleObjOld = pCurRoleObj;

				FinalIKALLUI* m_pCurUIOld = m_pCurUI;

				pCurRoleObj = iter->first;

				m_pCurUI = iter->second;

				SAFE_DELETE(iter->second);

				mAllRoleIKUI.erase(iter);

				pCurRoleObj = pCurRoleObjOld;

				m_pCurUI = m_pCurUIOld;

			}

			if (pCurRoleObj == pRoleObj) {

				if (IsLockRoleObj) {

					IsLockRoleObj = false;

				}

				pCurRoleObj = nullptr;

				m_pCurUI = nullptr;

			}

		}



		bool EditorFinalIKUI::UpdateRoleObject(RoleObject* pRoleObj) {

			if (IsLockRoleObj) {

				return false;

			}

			if (pRoleObj) {

				SetRoleObject(pRoleObj);

			}

			else {

				pCurRoleObj = nullptr;

			}

			return true;

		}

		void EditorFinalIKUI::SetRoleObject(RoleObject* pRoleObj) {

			if (!pRoleObj) {

				return;

			}

			if (pCurRoleObj != pRoleObj) {

				auto item = mAllRoleIKUI.find(pRoleObj);

				if (item == mAllRoleIKUI.end()) {

					m_pCurUI = new FinalIKALLUI(pRoleObj);

					mAllRoleIKUI[pRoleObj] = m_pCurUI;

					m_pCurUI->InitGui();

				}

				else {

					m_pCurUI = item->second;

				}

				pCurRoleObj = pRoleObj;

			}

		}



	};



};



namespace Echo {



	const ImVec4 gInteractorButtonColor(0.6f, 0.f, 0.f, 1.0f);

	const ImVec4 gInteractorButtonHoveredColor(1.0f, 0.f, 0.f, 1.0f);



	bool PhysicsManagerUI::IsCreated = false;

	bool PhysicsToolsUI::IsCreated = false;

	//bool InteractorManagerUI::IsCreated = false;



	void PhysicsManagerUI::InitGui(int classID)

	{

		mUIType = U8("物理管理界面");

		mUIName = U8("物理管理界面##") + std::to_string(classID);

		ImGui::SetNextWindowPos(mPosition, mPositionType);

		ImGui::SetNextWindowSize(mSize, mSizeType);	

	}



	void PhysicsManagerUI::Keyboard() {}





	PhysicsManagerUI::PhysicsManagerUI(int classID, ImGuiContext* ctx) 

		: BaseUI(classID, ctx)

		, mPosition(0.f, 0)

		, mPositionType(ImGuiCond_FirstUseEver)

		, mSize(300.f, 300)

		, mSizeType(ImGuiCond_FirstUseEver)

	{

		InitGui(classID);

		IsCreated = true;

	}

	PhysicsManagerUI::~PhysicsManagerUI() {

		IsCreated = false;

	}



	template<typename T>

	void PhysicsManagerUI::ShowComponentList(const char* label, const std::unordered_set<T*>& compList, char* pFilterData, uint32 filterDataSize) {

		int number = (int)compList.size();

		std::string Label = label;

		Label.append("(" + std::to_string(number) + ")");

		if (ImGui::TreeNodeEx(Label.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			ImGui::InputText("##filterName", pFilterData, filterDataSize, ImGuiInputTextFlags_EnterReturnsTrue);

			std::string fName(pFilterData);

			std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

			bool IsFilter = fName != "";

			std::list<std::pair<std::string, ActorComponent*>> showList;

			for (auto comp : compList) {

				ActorComponent* pComp = (ActorComponent*)comp;

				std::string nameStr(pComp->getCombinedName());

				if (!((PhysicsBaseComponent*)pComp)->IsExistPhysics()) nameStr.append(" *");

				std::string str = nameStr;

				std::transform(str.begin(), str.end(), str.begin(), ::tolower);

				if (!IsFilter || str.find(fName) != -1) {

					showList.push_back(std::make_pair(nameStr,pComp));

				}

			}

			ImGui::SameLine();

			ImGui::Text(("(" + std::to_string(showList.size()) + ")").c_str());

			for (auto comp : showList) {

				if (ImGui::TreeNodeEx(comp.first.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

					comp.second->ShowEditorComUI();

					ImGui::TreePop();

				}

			}

			ImGui::TreePop();

		}

	}



	static int ControllerBehaviorToIndex(uint32 value) {

		switch (value) {

		case 1:return 0;

		case 2:return 1;

		case 4:return 2;

		}

		return 0;

	}

	static uint32 IndexToControllerBehavior(int value) {

		switch (value) {

		case 0:return 1;

		case 1:return 2;

		case 2:return 4;

		}

		return 1;

	}



	void PhysicsManagerUI::ShowComponentPtrList(const char* label, const std::unordered_set<ActorComponentPtr>& compList, char* pFilterData, uint32 filterDataSize) {

		int number = (int)compList.size();

		std::string Label = label;

		Label.append("(" + std::to_string(number) + ")");

		if (ImGui::TreeNodeEx(Label.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			ImGui::InputText("##filterName", pFilterData, filterDataSize, ImGuiInputTextFlags_EnterReturnsTrue);

			std::string fName(pFilterData);

			std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

			bool IsFilter = fName != "";

			std::list<std::pair<std::string, ActorComponentPtr>> showList;

			for (const ActorComponentPtr& pComp : compList) {

				if (pComp.expired()) continue;

				std::string nameStr(pComp->getCombinedName());

				std::string str = nameStr;

				std::transform(str.begin(), str.end(), str.begin(), ::tolower);

				if (!IsFilter || str.find(fName) != -1) {

					showList.push_back(std::make_pair(nameStr, pComp));

				}

			}

			ImGui::SameLine();

			ImGui::Text(("(" + std::to_string(showList.size()) + ")").c_str());

			for (auto comp : showList) {

				if (ImGui::TreeNodeEx(comp.first.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

					comp.second->ShowEditorComUI();

					ImGui::TreePop();

				}

			}

			ImGui::TreePop();

		}

	}



	const ImVec4 ReadOnlyTextColor(0.6f, 0.6f, 0.6f, 1.0f);



	bool _CheckFilePath(std::string& path, const std::string& suffix, std::string folder, Name type = Name(""))

	{

		if (path.empty())

			return true;

		if (path.find(suffix) == path.npos)

		{

			std::string log = U8("[警告]: 请输入后缀为") + suffix + U8("的资源！");

			LogManager::instance()->logMessage(log);

			return false;

		}

		std::replace(path.begin(), path.end(), '\\', '/');

		std::replace(folder.begin(), folder.end(), '\\', '/');

		if (path.find(folder) == path.npos)

		{

			std::string log = U8("[警告]: 路径是否正确(") + folder + U8(")?");

			LogManager::instance()->logMessage(log);

			return  false;

		}



		std::string resRoot = Echo::Root::instance()->getRootResourcePath(type.toString());

		std::replace(resRoot.begin(), resRoot.end(), '\\', '/');

		size_t pos = path.find(resRoot);

		if (pos != std::string::npos)

		{

			path = path.erase(0, resRoot.size());

		}

		else

		{

			std::string resourceFolder = ResourceFolderManager::instance()->GetRootFolder(type);

			std::string filepath = path;

			if (filepath.find("asset/") != filepath.npos)

			{

				filepath.erase(0, filepath.find("asset/") + 6);

				if (!resourceFolder.empty() && filepath.find(resourceFolder) != filepath.npos)

					filepath.erase(0, filepath.find(resourceFolder) + strlen(resourceFolder.c_str()));

				std::string realpath = Echo::Root::instance()->getRootResourcePath() + filepath;

				if (_access(realpath.c_str(), 0) == 0)

					path = filepath;

				else

				{

					LogManager::instance()->logMessage(U8("资源不存在或资源根路径错误！"));

					return false;

				}

			}

			else

			{

				std::string filepath = resRoot + path;

				if (_access(filepath.c_str(), 0) == 0)

				{

					if (!resourceFolder.empty() && path.find(resourceFolder) != path.npos)

					{

						path.erase(0, path.find(resourceFolder) + strlen(resourceFolder.c_str()));

					}

					return true;

				}

				else {

					LogManager::instance()->logMessage(U8("资源不存在或资源根路径错误！"));

				}

				return false;

			}

		}

		return true;

	}



	void PhysicsManagerUI::ShowGui() {

		if (!ImGui::BeginChild(mUIName.c_str())) {

			ImGui::EndChild();

			return;

		}



		UI::SelectUI(nullptr, nullptr, mPhysicsManagerName, ImVec2(ImGui::GetWindowWidth(), 0.f), [](std::vector<const char*>& optionVector, void* pUserData) {

				auto& mp = EchoPhysicsSystem::instance()->GetManagerMap();

				for (auto& iter : mp) {

					optionVector.push_back(iter.first.c_str());

				}

			}, __LINE__, mFilterData[14], 256);

		EchoPhysicsManager* pPhysicsManager = EchoPhysicsSystem::instance()->GetManager(mPhysicsManagerName.empty() ? WorldSystem::instance()->GetActiveWorld() : mPhysicsManagerName);

		if (pPhysicsManager) {

		if (pPhysicsManager->PhysicsIsRunning()) {

			ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::LockRoleButtonColor);

			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::LockRoleButtonHoveredColor);

			if (ImGui::Button(U8("正在运行"), ImVec2(ImGui::GetWindowWidth(), 0.f))) {

				pPhysicsManager->SetRunPhysics(false);

			}

			ImGui::PopStyleColor(2);

		}

		else if (ImGui::Button(U8("未运行"), ImVec2(ImGui::GetWindowWidth(), 0.f))) {

			pPhysicsManager->SetRunPhysics(true);

		}



		if (pPhysicsManager->PhysicsIsUpdating()) {

			ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::LockRoleButtonColor);

			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::LockRoleButtonHoveredColor);

			if (ImGui::Button(U8("正在场景模拟"), ImVec2(ImGui::GetWindowWidth(), 0.f))) {

				pPhysicsManager->SetUpdatePhysics(false);

			}

			ImGui::PopStyleColor(2);

		}

		else if (ImGui::Button(U8("未场景模拟"), ImVec2(ImGui::GetWindowWidth(), 0.f))) {

			pPhysicsManager->SetUpdatePhysics(true);

		}



		if (ImGui::TreeNodeEx(U8("物理组件"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

		

			ShowComponentList("RigidBody", pPhysicsManager->GetAllRigidBody(), mFilterData[0], 255);

			ShowComponentList("Aggregate", pPhysicsManager->GetAllAggregate(), mFilterData[11], 255);

			ShowComponentList("Collider", pPhysicsManager->GetAllCollider(), mFilterData[1], 255);

			ShowComponentList("Joint", pPhysicsManager->GetAllJoint(), mFilterData[2], 255);

			ShowComponentList("Force", pPhysicsManager->GetAllForce(), mFilterData[3], 255);

			ShowComponentList("CharactorController", pPhysicsManager->GetAllController(), mFilterData[5], 255);

			ShowComponentList("TerrainCollider", pPhysicsManager->GetAllTerrain(), mFilterData[8], 255);

		

			ImGui::TreePop();

		}



#ifdef USE_PHYSICS_PAGE

		if (ImGui::TreeNodeEx(U8("物理分页"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			bool bUsePhysXPage = pPhysicsManager->getUsePhysXPage();

			if (ImGui::Checkbox("Is Use PhysXPage", &bUsePhysXPage)) {

				pPhysicsManager->setUsePhysXPage(bUsePhysXPage);

			}

			int asyncLoadNum = pPhysicsManager->GetAsyncLoadNumber();

			if (ImGui::InputInt("Frame Async Load Number", &asyncLoadNum)) {

				pPhysicsManager->SetAsyncLoadNumber(asyncLoadNum);

			}

			{

				if (ImGui::TreeNodeEx("PhysXPageLoadMainActor", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

					int delIndex = -1;

					int size = pPhysicsManager->GetPhysXPageLoadMainActorNumber();

					for (int i = 0; i != size; ++i) {

						PhysXPageLoadPointData data = pPhysicsManager->GetPhysXPageLoadMainActor(i);

						String mainActorPtrName;

						if (data.m_pCompPtr != nullptr) {

							mainActorPtrName = data.m_pCompPtr->getCombinedName();

						}

						bool bChange = false;

						if (UI::SelectUI("", nullptr, mainActorPtrName, ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth * 0.7f, 0.f), [](std::vector<String>& optionVector, void* pUserData) {

							std::vector<ActorPtr> actors = ActorSystem::instance()->getActiveActorManager()->getAllActor();

						for (auto actor : actors) {

							const std::vector<ActorComponentPtr>& components = actor->getEditorActorComs();

							for (auto comp : components) {

								if (comp == nullptr) continue;

								optionVector.push_back(comp->getCombinedName());

							}

						}

							}, __LINE__ + i, mFilterData[13], 256)) {

							PickActorResult res = ActorSystem::instance()->getActiveActorManager()->GetActorComBySeletedName(mainActorPtrName, true);

							bChange = true;

							data.m_pCompPtr = res.seleteCom;

						}

						ImGui::SameLine();

						if (ImGui::Checkbox(("##ForceAsync" + std::to_string(i)).c_str(), &data.m_bForceAsync)) {

							bChange = true;

						}

						ImGui::SameLine();

						if (ImGui::Button(("DEL##RemoveLoadTarget" + std::to_string(i)).c_str(), ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth * 0.3f, 0.f))) {

							delIndex = i;

							bChange = false;

						}

						ImGui::SameLine();

						ImGui::Text(std::to_string(i).c_str());

						if (bChange) {

							pPhysicsManager->SetPhysXPageLoadMainActor(i, data.m_pCompPtr, data.m_bForceAsync);

						}

					}

					if (ImGui::Button("ADD", ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth, 0.f))) {

						pPhysicsManager->AddPhysXPageLoadMainActor(nullptr);

					}

					if (delIndex != -1) {

						pPhysicsManager->RemovePhysXPageLoadMainActor(delIndex);

					}

					ImGui::TreePop();

				}



				auto pPageMgr = pPhysicsManager->getMapPagedLayers(ePhysXStaticObjectType);

				if (pPageMgr != nullptr && ImGui::TreeNodeEx("Static Page Data", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



					bool change = false;



					float cellSize = pPageMgr->getCellSize();

					if (ImGui::InputFloat("Cell Size", &cellSize)) {

						change = true;

					}



					int asyncLoadNum = pPageMgr->mFrameAsyncLoadNumber;

					if (ImGui::InputInt("Frame Async Load Number", &asyncLoadNum)) {

						pPageMgr->mFrameAsyncLoadNumber = std::max(0, asyncLoadNum);

					}



					float mAsyncLoadWidth = pPageMgr->getAsyncLoadWidth();

					if (ImGui::InputFloat("Async Load Width", &mAsyncLoadWidth)) {

						change = true;

					}



					int mPageSize = pPageMgr->getPageSize();

					if (ImGui::InputInt("Load Half Size", &mPageSize)) {

						change = true;

					}



					if (change && !pPhysicsManager->PhysicsIsRunning()) {

						pPageMgr->SetPageSize(cellSize, asyncLoadNum, mAsyncLoadWidth, mPageSize);

					}

#ifndef PHYSICS_RUNTIME

					if (ImGui::TreeNodeEx("Object", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

						ImGui::InputText("##filterName", mFilterData[10], 256, ImGuiInputTextFlags_EnterReturnsTrue);

						std::string fName(mFilterData[10]);

						std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

						bool IsFilter = fName != "";

						std::list<std::string> showList;

						for (auto comp : EchoPhysXStaticObject::gAllObject) {

							ColliderComponent* pComp = comp->m_pComp;

							if (pComp->GetPhysicsManager() != pPhysicsManager) continue;

							std::string nameStr(pComp->getCombinedName());

							if (!pComp->IsExistPhysics()) nameStr.append(" *");

							std::string str = nameStr;

							std::transform(str.begin(), str.end(), str.begin(), ::tolower);

							if (!IsFilter || str.find(fName) != -1) {

								showList.push_back(nameStr);

							}

						}

						ImGui::SameLine();

						ImGui::Text(("(" + std::to_string(showList.size()) + ")").c_str());

						for (auto& comp : showList) {

							ImGui::Text(comp.c_str());

						}

						ImGui::TreePop();

					}



#endif // !PHYSICS_RUNTIME

					ImGui::TreePop();

				}



				pPageMgr = pPhysicsManager->getMapPagedLayers(ePhysXDynamicObjectType);

				if (pPageMgr != nullptr && ImGui::TreeNodeEx("Dynamic Page Data", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



					bool change = false;



					float cellSize = pPageMgr->getCellSize();

					if (ImGui::InputFloat("Cell Size", &cellSize)) {

						change = true;

					}



					int asyncLoadNum = pPageMgr->mFrameAsyncLoadNumber;

					if (ImGui::InputInt("Frame Async Load Number", &asyncLoadNum)) {

						pPageMgr->mFrameAsyncLoadNumber = std::max(0, asyncLoadNum);

					}



					float mAsyncLoadWidth = pPageMgr->getAsyncLoadWidth();

					if (ImGui::InputFloat("Async Load Width", &mAsyncLoadWidth)) {

						change = true;

					}



					int mPageSize = pPageMgr->getPageSize();

					if (ImGui::InputInt("Load Half Size", &mPageSize)) {

						change = true;

					}



					if (change && !pPhysicsManager->PhysicsIsRunning()) {

						pPageMgr->SetPageSize(cellSize, asyncLoadNum, mAsyncLoadWidth, mPageSize);

					}



#ifndef PHYSICS_RUNTIME

					if (ImGui::TreeNodeEx("Object", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

						ImGui::InputText("##filterName", mFilterData[10], 256, ImGuiInputTextFlags_EnterReturnsTrue);

						std::string fName(mFilterData[10]);

						std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

						bool IsFilter = fName != "";

						std::list<std::string> showList;

						for (auto comp : EchoPhysXDynamicObject::gAllObject) {

							RigidBodyComponent* pComp = comp->m_pComp;

							if (pComp->GetPhysicsManager() != pPhysicsManager) continue;

							std::string nameStr(pComp->getCombinedName());

							if (!pComp->IsExistPhysics()) nameStr.append(" *");

							std::string str = nameStr;

							std::transform(str.begin(), str.end(), str.begin(), ::tolower);

							if (!IsFilter || str.find(fName) != -1) {

								showList.push_back(nameStr);

							}

						}

						ImGui::SameLine();

						ImGui::Text(("(" + std::to_string(showList.size()) + ")").c_str());

						for (auto& comp : showList) {

							ImGui::Text(comp.c_str());

						}

						ImGui::TreePop();

					}





#endif // !PHYSICS_RUNTIME

					ImGui::TreePop();

				}



				pPageMgr = pPhysicsManager->getMapPagedLayers(ePhysXGroupObjectType);

				if (pPageMgr != nullptr && ImGui::TreeNodeEx("Group Page Data", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



					bool change = false;



					float cellSize = pPageMgr->getCellSize();

					if (ImGui::InputFloat("Cell Size", &cellSize)) {

						change = true;

					}



					int asyncLoadNum = pPageMgr->mFrameAsyncLoadNumber;

					if (ImGui::InputInt("Frame Async Load Number", &asyncLoadNum)) {

						pPageMgr->mFrameAsyncLoadNumber = std::max(0, asyncLoadNum);

					}



					float mAsyncLoadWidth = pPageMgr->getAsyncLoadWidth();

					if (ImGui::InputFloat("Async Load Width", &mAsyncLoadWidth)) {

						change = true;

					}



					int mPageSize = pPageMgr->getPageSize();

					if (ImGui::InputInt("Load Half Size", &mPageSize)) {

						change = true;

					}



					if (change && !pPhysicsManager->PhysicsIsRunning()) {

						pPageMgr->SetPageSize(cellSize, asyncLoadNum, mAsyncLoadWidth, mPageSize);

					}



#ifndef PHYSICS_RUNTIME

					if (ImGui::TreeNodeEx("Object", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

						std::list<std::list<std::string>> showList;

						for (auto comp : EchoPhysXGroupObject::gAllObject) {

							auto& curList = showList.emplace_back();

							curList.push_back("Group" + std::to_string((int)showList.size()));

							for (auto& iter : comp->m_pGraph->mData) {

								if (iter.first->getPhysXType() == ePhysXStaticObjectType) {

									EchoPhysXStaticObject* pObj = (EchoPhysXStaticObject*)iter.first;

									ColliderComponent* pComp = pObj->m_pComp;

									if (pComp->GetPhysicsManager() != pPhysicsManager) continue;

									std::string nameStr(pComp->getCombinedName());

									if (!pComp->IsExistPhysics()) nameStr.append(" *");

									curList.push_back(nameStr);

								}

								else {

									EchoPhysXDynamicObject* pObj = (EchoPhysXDynamicObject*)iter.first;

									RigidBodyComponent* pComp = pObj->m_pComp;

									if (pComp->GetPhysicsManager() != pPhysicsManager) continue;

									std::string nameStr(pComp->getCombinedName());

									if (!pComp->IsExistPhysics()) nameStr.append(" *");

									curList.push_back(nameStr);

								}

							}

						}

						for (auto& comp : showList) {

							auto iter = comp.begin();

							if (ImGui::TreeNodeEx(iter->c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

								++iter;

								while (iter != comp.end()) {

									ImGui::Text(iter->c_str());

									++iter;

								}

								ImGui::TreePop();

							}

						}

						ImGui::TreePop();

					}





#endif // !PHYSICS_RUNTIME



					ImGui::TreePop();

				}





			}



			{



				if (ImGui::TreeNodeEx("Page Layers", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



					std::map<float, uint32> datas;



					ImGui::Text("Static");

					pPhysicsManager->calculatePagedLayersData(datas, ePhysXStaticObjectType);

					for (auto& data : datas) {

						ImGui::Text(std::to_string(data.first).c_str());

						ImGui::SameLine();

						ImGui::Text(std::to_string(data.second).c_str());

					}



					ImGui::Text("Dynamic");

					pPhysicsManager->calculatePagedLayersData(datas, ePhysXDynamicObjectType);

					for (auto& data : datas) {

						ImGui::Text(std::to_string(data.first).c_str());

						ImGui::SameLine();

						ImGui::Text(std::to_string(data.second).c_str());

					}



					ImGui::Text("Group");

					pPhysicsManager->calculatePagedLayersData(datas, ePhysXGroupObjectType);

					for (auto& data : datas) {

						ImGui::Text(std::to_string(data.first).c_str());

						ImGui::SameLine();

						ImGui::Text(std::to_string(data.second).c_str());

					}



					ImGui::TreePop();

				}

			}



			ImGui::TreePop();

		}

#endif //USE_PHYSICS_PAGE

		if (ImGui::TreeNodeEx(U8("物理参数"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			ImGui::Text("EchoEngine Physics");

			

			float FixedTimeInv = 1.0f / PhysXWorldSystem::instance()->GetFixedTime();

			if (ImGui::InputFloat("Physics Frame Rate", &FixedTimeInv, 0.0f, 0.0f, "%.3f")) {

				PhysXWorldSystem::instance()->SetFixedTime(1.0f / FixedTimeInv);

			}



#ifdef USE_PHYSICS_HELPEROBJ

			ImGui::Checkbox("Update Render", &bUpdateComponentInEditor);

			if (bUpdateComponentInEditor) {

				pPhysicsManager->UpdateComponentInEditor();

			}

#endif // USE_PHYSICS_HELPEROBJ



			ImGui::Text("Scene");



			Vector3 gravity = pPhysicsManager->GetGravity();

			if (ImGui::InputFloat3("Gravity", gravity.ptr())) {

				pPhysicsManager->SetGravity(gravity);

			}



			Vector3 worldOrigin = pPhysicsManager->GetScene()->GetPhysXWorldOrigin();

			if (ImGui::InputFloat3("Physics World Origin", worldOrigin.ptr())) {

				pPhysicsManager->GetScene()->SetPhysXWorldOrigin(worldOrigin);

			}



			ImGui::Text("ControllerManager");



			{

				uint32 flag = pPhysicsManager->GetDefaultControllerBehavior();

				std::vector<const char*> items = {

					"eCCT_CAN_RIDE_ON_OBJECT",

					"eCCT_SLIDE",

					"eCCT_USER_DEFINED_RIDE"

				};

				int index = ControllerBehaviorToIndex(flag);

				if (ImGui::Combo("Default Controller Behavior", &index, items.data(), 3)) {

					pPhysicsManager->SetDefaultControllerBehavior(IndexToControllerBehavior(index));

				}

			}



			//bool mbResetCCT = pPhysicsManager->GetIsResetCCT();

			//if (ImGui::Checkbox("Is Auto Reset World Position", &mbResetCCT)) {

			//	pPhysicsManager->SetIsResetCCT(mbResetCCT);

			//}



			bool mbControllerOverlapRecoveryModule = pPhysicsManager->GetControllerOverlapRecoveryModule();

			if (ImGui::Checkbox("Is Overlap Recovery", &mbControllerOverlapRecoveryModule)) {

				pPhysicsManager->SetControllerOverlapRecoveryModule(mbControllerOverlapRecoveryModule);

			}

			bool mbControllerPreciseSweeps = pPhysicsManager->GetControllerPreciseSweeps();

			if (ImGui::Checkbox("Is Precise Sweeps", &mbControllerPreciseSweeps)) {

				pPhysicsManager->SetControllerPreciseSweeps(mbControllerPreciseSweeps);

			}

			bool mbControllerPreventVerticalSlidingAgainstCeiling = pPhysicsManager->GetControllerPreventVerticalSlidingAgainstCeiling();

			if (ImGui::Checkbox("Is Prevent Vertical Sliding Against Ceiling", &mbControllerPreventVerticalSlidingAgainstCeiling)) {

				pPhysicsManager->SetControllerPreventVerticalSlidingAgainstCeiling(mbControllerPreventVerticalSlidingAgainstCeiling);

			}



			bool mbMeshDoubleSided = pPhysicsManager->GetMeshDoubleSided();

			if (ImGui::Checkbox("Mesh Double Sided", &mbMeshDoubleSided)) {

				pPhysicsManager->SetMeshDoubleSided(mbMeshDoubleSided);

			}



			bool mIsSyncGlobalPose = pPhysicsManager->GetControllerIsSyncGlobalPose();

			if (ImGui::Checkbox("Is Sync GlobalPose After Move", &mIsSyncGlobalPose)) {

				pPhysicsManager->SetControllerIsSyncGlobalPose(mIsSyncGlobalPose);

			}



			{

				bool isChange = false;

				std::pair<bool, float> mControllerTessellation = pPhysicsManager->GetControllerTessellation();

				if (ImGui::Checkbox("Is Tessellation", &mControllerTessellation.first)) {

					isChange = true;

				}

				if (ImGui::DragFloat("Tessellation MaxEdgeLength", &mControllerTessellation.second, 0.01f, 0.0f, 10.0f)) {

					isChange = true;

				}

				if (isChange) {

					pPhysicsManager->SetControllerTessellation(mControllerTessellation.first, mControllerTessellation.second);

				}

			}



			ImGui::TreePop();

		}

		

		if (ImGui::TreeNodeEx(U8("物理辅助线"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



			if (pPhysicsManager->GetPhysXWorldManager()->GetShowDebugVisualization()) {

				ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::LockRoleButtonColor);

				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::LockRoleButtonHoveredColor);

				if (ImGui::Button(U8("正在绘制"), ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth, 0.f))) {

					pPhysicsManager->GetPhysXWorldManager()->SetShowDebugVisualization(false);

				}

				ImGui::PopStyleColor(2);

			}

			else if (ImGui::Button(U8("未绘制"), ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth, 0.f))) {

				pPhysicsManager->GetPhysXWorldManager()->SetShowDebugVisualization(true);

			}



			PhysXSceneImpl* pPhysXScene = pPhysicsManager->GetScene();



			auto showVisParmFun = [pPhysXScene](const char* label, EchoVisualizationParameter type) {

				float value = pPhysXScene->GetVisualizationParameter(type);

				if (ImGui::DragFloat(label, &value, 0.01f, 0.0f, 100.0f)) {

					pPhysXScene->SetVisualizationParameter(type, value);

				}

			};



			auto showVisParmBool2Fun = [pPhysXScene](const char* label, EchoVisualizationParameter type) {

				bool value = pPhysXScene->GetVisualizationParameter(type) != 0.0f;

				if (ImGui::Checkbox(label, &value)) {

					pPhysXScene->SetVisualizationParameter(type, value ? 1.0f : 0.0f);

				}

			};



			auto showVisParmBoolFun = [pPhysXScene](const char* label, EchoVisualizationParameter type) {

				bool bValue = pPhysXScene->GetVisualizationParameter(type) != 0.0f;

				if (bValue) {

					ImGui::PushID(label);

					float value = pPhysXScene->GetVisualizationParameter(type);

					if (ImGui::DragFloat("##VisualizationParameterFloatValue", &value, 0.01f, 0.0f, 100.0f)) {

						pPhysXScene->SetVisualizationParameter(type, value);

					}

					ImGui::SameLine();

					if (ImGui::Checkbox(label, &bValue)) {

						pPhysXScene->SetVisualizationParameter(type, bValue ? 1.0f : 0.0f);

					}

					ImGui::PopID();

				}

				else if (ImGui::Checkbox(label, &bValue)) {

					pPhysXScene->SetVisualizationParameter(type, bValue ? 1.0f : 0.0f);

				}

			};





			ImGui::Text(U8("可视化剔除"));

			Vector3 halfExtents = pPhysicsManager->GetPhysXWorldManager()->GetCullingBoxHalfExtents();

			if (ImGui::DragFloat3(U8("剔除框半长"), halfExtents.ptr(), 1.0f, 0.0f)) {

				pPhysicsManager->GetPhysXWorldManager()->SetCullingBoxHalfExtents(halfExtents);

			}

			showVisParmBool2Fun(U8("显示剔除框"), EchoVisualizationParameter::eCULL_BOX);

			ImGui::SameLine();

			bool manualCulling = pPhysicsManager->GetPhysXWorldManager()->GetIsManualCulling();

			if (ImGui::Checkbox(U8("剔除坐标轴包围盒"), &manualCulling)) {

				pPhysicsManager->GetPhysXWorldManager()->SetIsManualCulling(manualCulling);

			}

			ImGui::Text(U8("场景"));

			showVisParmBoolFun(U8("场景坐标轴"), EchoVisualizationParameter::eWORLD_AXES);

			showVisParmBool2Fun(U8("静态剪枝包围盒树"), EchoVisualizationParameter::eCOLLISION_STATIC);

			showVisParmBool2Fun(U8("动态剪枝包围盒树"), EchoVisualizationParameter::eCOLLISION_DYNAMIC);

			ImGui::Text(U8("刚体"));

			showVisParmBoolFun(U8("刚体坐标轴"), EchoVisualizationParameter::eACTOR_AXES);

			showVisParmBoolFun(U8("动态刚体质心坐标轴"), EchoVisualizationParameter::eBODY_AXES);

			showVisParmBool2Fun(U8("动态刚体质心几何体"), EchoVisualizationParameter::eBODY_MASS_AXES);

			showVisParmBoolFun(U8("动态刚体线速度"), EchoVisualizationParameter::eBODY_LIN_VELOCITY);

			showVisParmBoolFun(U8("动态刚体角速度"), EchoVisualizationParameter::eBODY_ANG_VELOCITY);

			ImGui::Text(U8("碰撞体"));

			showVisParmBool2Fun(U8("碰撞体"), EchoVisualizationParameter::eCOLLISION_SHAPES);

			showVisParmBoolFun(U8("碰撞体坐标轴"), EchoVisualizationParameter::eCOLLISION_AXES);

			showVisParmBool2Fun(U8("碰撞体包围盒"), EchoVisualizationParameter::eCOLLISION_AABBS);

			showVisParmBool2Fun(U8("复合碰撞体包围盒"), EchoVisualizationParameter::eCOLLISION_COMPOUNDS);

			showVisParmBoolFun(U8("三角形网格碰撞体法线"), EchoVisualizationParameter::eCOLLISION_FNORMALS);

			showVisParmBoolFun(U8("三角形网格碰撞体边线"), EchoVisualizationParameter::eCOLLISION_EDGES);

			ImGui::Text(U8("关节"));

			showVisParmBoolFun(U8("关节坐标系"), EchoVisualizationParameter::eJOINT_LOCAL_FRAMES);

			showVisParmBoolFun(U8("关节限制"), EchoVisualizationParameter::eJOINT_LIMITS);

			ImGui::Text(U8("碰撞信息"));

			showVisParmBool2Fun(U8("接触点"), EchoVisualizationParameter::eCONTACT_POINT);

			showVisParmBoolFun(U8("接触法线"), EchoVisualizationParameter::eCONTACT_NORMAL);

			showVisParmBoolFun(U8("接触力"), EchoVisualizationParameter::eCONTACT_FORCE);

			showVisParmBool2Fun(U8("接触错误"), EchoVisualizationParameter::eCONTACT_ERROR);



			auto showCCTVisParmBoolFun = [pPhysXScene](const char* label, EchoControllerDebugRenderFlags& value, EchoControllerDebugRenderFlag type) {

				bool bValue = value & (uint32)type;

				if (ImGui::Checkbox(label, &bValue)) {

					if (bValue) {

						value = value | (uint32)type;

					}

					else {

						value = value & (~(uint32)type);

					}

				}

			};



			ImGui::Text(U8("角色控制器"));

			EchoControllerDebugRenderFlags flags = pPhysXScene->GetCCTVisualizationFlags();

			showCCTVisParmBoolFun(U8("时间包围盒"), flags, EchoControllerDebugRenderFlag::eTEMPORAL_BV);

			showCCTVisParmBoolFun(U8("缓存包围盒"), flags, EchoControllerDebugRenderFlag::eCACHED_BV);

			showCCTVisParmBoolFun(U8("缓存刚体包围盒"), flags, EchoControllerDebugRenderFlag::eCACHEDGEOM_BV);

			if (flags != pPhysXScene->GetCCTVisualizationFlags()) {

				pPhysXScene->SetCCTVisualizationFlags(flags);

			}



			ImGui::TreePop();

		}



//		ShowComponentPtrList(U8("物理更新队列"), pPhysicsManager->mPhysicsUpdateQueue, mFilterData[6], 255);

//#ifdef USE_PHYSICS_HELPEROBJ

//		ShowComponentPtrList(U8("编辑更新队列"), pPhysicsManager->mEditorUpdateQueue, mFilterData[7], 255);

//#endif // USE_PHYSICS_HELPEROBJ



		if (ImGui::TreeNodeEx(U8("实时数据"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

	

			int curMoveNum = pPhysicsManager->GetCurrentMoveNumber();

			ImGui::Text(("RigidBody Move Number : " + std::to_string(curMoveNum)).c_str());

			int curCollisionNum = pPhysicsManager->GetCurrentCollisionNumber();

			ImGui::Text(("Collider Collision Number : " + std::to_string(curCollisionNum)).c_str());

			int curTriggerNum = pPhysicsManager->GetCurrentTriggerNumber();

			ImGui::Text(("Collider Trigger Number : " + std::to_string(curTriggerNum)).c_str());



			int phyUpdateNumber = (int)pPhysicsManager->mPhysicsUpdateQueue.size();

			ImGui::Text(("Physics Update Number : " + std::to_string(phyUpdateNumber)).c_str());

			int ediUpdateNumber = (int)pPhysicsManager->mEditorUpdateQueue.size();

			ImGui::Text(("Editor Update Number : " + std::to_string(ediUpdateNumber)).c_str());

			

			int nR = pPhysicsManager->GetRigidBody_Exist();

			ImGui::Text(("RigidBody Exist Number : " + std::to_string(nR)).c_str());

			int nC = pPhysicsManager->GetCollider_Exist();

			ImGui::Text(("Collider Exist Number : " + std::to_string(nC)).c_str());

			int nT = pPhysicsManager->GetTerrain_Exist();

			ImGui::Text(("Terrain Exist Number : " + std::to_string(nT)).c_str());

			int nJ = pPhysicsManager->GetJoint_Exist();

			ImGui::Text(("Joint Exist Number : " + std::to_string(nJ)).c_str());

			int nF = pPhysicsManager->GetForce_Exist();

			ImGui::Text(("Force Exist Number : " + std::to_string(nF)).c_str());

			int nCCT = pPhysicsManager->GetController_Exist();

			ImGui::Text(("Controller Exist Number : " + std::to_string(nCCT)).c_str());

			int nA = pPhysicsManager->GetAggregate_Exist();

			ImGui::Text(("Aggregate Exist Number : " + std::to_string(nA)).c_str());



			ImGui::TreePop();

		}



		}



		if (ImGui::TreeNodeEx(U8("物理分组"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



			if (ImGui::TreeNodeEx(U8("Bit Group"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



				auto& group1Name = PhysXGroup::GetGroupName();



				float intervalWidth = ImGui::GetWindowWidth() * 0.4f;



				char buffer[500] = { '\0' };

				for (int i = 0; i != group1Name.size(); ++i) {

					ImGui::PushID(i);

					std::string str;

					if (i < 4) {

						str = "Engine Group " + std::to_string(i);

						ImGui::PushStyleColor(ImGuiCol_Text, ReadOnlyTextColor);

					}

					else {

						str = "Client Group " + std::to_string(i);

					}

					ImGui::Text(str.c_str());

					ImGui::SameLine(intervalWidth);

					std::strncpy(buffer, group1Name[i].c_str(), group1Name[i].size() + 1);



					ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;

					if (i < 4) {

						flags |= ImGuiInputTextFlags_ReadOnly;

					}

					if (ImGui::InputText("##InputText", buffer, 500, flags)) {

						PhysXGroup::SetBitName(i, Name(buffer));

					}

					if (i < 4) {

						ImGui::PopStyleColor(1);

					}

					ImGui::PopID();

				}



				ImGui::TreePop();

			}



			if (ImGui::TreeNodeEx(U8("Filter Group"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



				auto& maskToNameMap = PhysXGroup::GetGroup2MaskToName();



				float intervalWidth = ImGui::GetWindowWidth() * 0.4f;



				char buffer[500] = { '\0' };

				ImVec2 size = ImGui::GetWindowSize();

				int id = 0;

				for (auto& group : maskToNameMap) {

					for (auto& name : group.second) {

						bool bChange = !PhysXGroup::IsSave(name);

						if (bChange) {

							ImGui::PushStyleColor(ImGuiCol_Text, ReadOnlyTextColor);

						}

						ImGui::PushID(id++);

						ImGui::Text(std::to_string(group.first).c_str());

						ImGui::SameLine(intervalWidth);

						std::strncpy(buffer, name.c_str(), name.size() + 1);

						ImGui::InputText("##InputText", buffer, 500, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_EnterReturnsTrue);

						ImGui::PopID();

						if (bChange) {

							ImGui::PopStyleColor(1);

						}



					}

				}





				if (ImGui::ButtonEx(std::string(std::to_string(mStaticPair.first) + "##CanInteractFilterGroupValue").c_str(), ImVec2(size.x * 0.323f, 0.f))) {

					ImGui::OpenPopup("##mStaticPairFirst");

				}

				ImGui::SetNextWindowSize(ImVec2(size.x * 0.5f, 0.f));

				if (ImGui::BeginPopup("##mStaticPairFirst")) {



					uint32 newCanInteractFilterGroup = mStaticPair.first;

					ImGui::SetNextItemWidth(size.x * 0.25f);

					if (ImGui::InputInt("##CanInteractFilterGroupvalue", (int*)&newCanInteractFilterGroup, 1, 1000, ImGuiInputTextFlags_CharsHexadecimal)) {

						mStaticPair.first = newCanInteractFilterGroup;

					}

					ImGui::SameLine();

					if (ImGui::Button("All Zero", ImVec2(size.x * 0.10f, 0.f))) {

						mStaticPair.first = 0x0;

					}

					ImGui::SameLine();

					if (ImGui::Button("All One", ImVec2(size.x * 0.10f, 0.f))) {

						mStaticPair.first = 0xffffffff;

					}

					newCanInteractFilterGroup = 0x0;

					uint32 FilterGroupBit = 0x1;

					float intervalWidth = size.x * 0.1f;

					for (int i = 0; i != 32; ++i) {

						bool IsSelect = (mStaticPair.first & FilterGroupBit) != 0;

						ImGui::Checkbox(std::to_string(i).c_str(), &IsSelect);

						if (IsSelect) {

							newCanInteractFilterGroup |= FilterGroupBit;

						}

						FilterGroupBit = FilterGroupBit << 1;

						if ((i + 1) % 4 != 0) {

							ImGui::SameLine(intervalWidth * ((i + 1) % 4));

						}

					}

					mStaticPair.first = newCanInteractFilterGroup;

					ImGui::EndPopup();

				}





				ImGui::SameLine();



				std::strncpy(buffer, mStaticPair.second.c_str(), mStaticPair.second.size() + 1);

				ImGui::SetNextItemWidth(size.x * 0.323f);

				if (ImGui::InputText("##Group", buffer, 500)) {

					mStaticPair.second = Name(buffer);

				}

				ImGui::SameLine();

				if (ImGui::Button(U8("添加组"))) {

					PhysXGroup::AddGroup(mStaticPair.first, mStaticPair.second);

					PhysXGroup::AddSaveGroupName(mStaticPair.second);

				}

				ImGui::SameLine();

				if (ImGui::Button(U8("删除组"))) {

					PhysXGroup::RemoveGroup(mStaticPair.second);

					PhysXGroup::RemoveSaveGroupName(mStaticPair.second);

				}



				ImGui::TreePop();

			}



			if (ImGui::Button(U8("重载配置"), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.0f))) {



				PhysXWorldSystem::instance()->ImportPhysXGroup();

			}



			if (ImGui::Button(U8("保存配置"), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.0f))) {



				PhysXWorldSystem::instance()->ExportPhysXGroup();

			}

			ImGui::TreePop();

		}

		if (ImGui::TreeNodeEx(U8("物理颜色"), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



			auto ShowColorSelect = [](const char* label, int& index) {

				static std::vector<const char*> colors = {

					"HelperObjColorType_Red",		// {255	0	0	}

					"HelperObjColorType_Green",		// {0	255	0	}

					"HelperObjColorType_Blue",		// {0	0	255	}

					"helperObjColorType_Aqua",		// {0	255	255	}

					"helperObjColorType_Magenta",		// {255	0	255	}

					"helperObjColorType_Yellow",		// {255	255	0	}

					"HelperObjColorType_white",		// {255	255	255	}

					"HelperObjColorType_black",		// {0	0	0	}

					"HelperObjColorTypeEx_Azure",             // {240,255,255}

					"HelperObjColorTypeEx_Brown",             // {165,42,42}

					"HelperObjColorTypeEx_Coral",             // {255,127,80}

					"HelperObjColorTypeEx_Darkblue",          // {0,0,139}

					"HelperObjColorTypeEx_Darkgrey",          // {169,169,169}

					"HelperObjColorTypeEx_Darkorchid",        // {153,50,204}

					"HelperObjColorTypeEx_Darkslategray",     // {47,79,79}

					"HelperObjColorTypeEx_Deepskyblue",       // {0,191,255}

					"HelperObjColorTypeEx_Floralwhite",       // {255,250,240}

					"HelperObjColorTypeEx_Gold",              // {255,215,0}

					"HelperObjColorTypeEx_Honeydew",          // {240,255,240}

					"HelperObjColorTypeEx_Khaki",             // {240,230,140}

					"HelperObjColorTypeEx_Lightblue",         // {173,216,230}

					"HelperObjColorTypeEx_Lightgreen",        // {144,238,144}

					"HelperObjColorTypeEx_Lightskyblue",      // {135,206,250}

					"HelperObjColorTypeEx_Limegreen",         // {50,205,50}

					"HelperObjColorTypeEx_Mediumorchid",      // {186,85,211}

					"HelperObjColorTypeEx_Mediumturquoise",   // {72,209,204}

					"HelperObjColorTypeEx_Moccasin",          // {255,228,181}

					"HelperObjColorTypeEx_Olivedrab",         // {107,142,35}

					"HelperObjColorTypeEx_Palegreen",         // {152,251,152}

					"HelperObjColorTypeEx_Peru",              // {205,133,63}

					"HelperObjColorTypeEx_Rosybrown",         // {188,143,143}

					"HelperObjColorTypeEx_Seagreen",          // {46,139,87}

					"HelperObjColorTypeEx_Slateblue",         // {106,90,205}

					"HelperObjColorTypeEx_Steelblue",         // {70,130,180}

					"HelperObjColorTypeEx_Turquoise"         // {64,224,208}

				};

				int size = (int)colors.size();

				if (ImGui::Combo(label, &index, colors.data(), size)) {

					return true;

				}

				return false;

			};



			ShowColorSelect("Box", PhysXColor::gBox);

			ShowColorSelect("Sphere", PhysXColor::gSphere);

			ShowColorSelect("Capsule", PhysXColor::gCapsule);

			ShowColorSelect("ConvexMesh", PhysXColor::gConvexMesh);

			ShowColorSelect("TriangleMesh", PhysXColor::gTriangleMesh);

			ShowColorSelect("CharactorController", PhysXColor::gCharactorController);

			ShowColorSelect("MathCharactorController", PhysXColor::gMathCharactorController);

			ShowColorSelect("Terrain", PhysXColor::gTerrain);

			ShowColorSelect("NotExistPhysics", PhysXColor::gNotExistPhysics);



			if (ImGui::Button(U8("重置场景颜色"), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.0f))) {

				EchoPhysicsSystem::instance()->ResetAllColliderColor();

			}



			if (ImGui::Button(U8("重载配置"), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.0f))) {



				PhysXWorldSystem::instance()->ImportPhysXColor();

			}



			if (ImGui::Button(U8("保存配置"), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.0f))) {



				PhysXWorldSystem::instance()->ExportPhysXColor();

			}

			ImGui::TreePop();

		}



		ImGui::EndChild();

	}





	PhysXMeshSerializer& _GetPhysXMeshSerializer() {

		static PhysXMeshSerializer gSerializer;

		return gSerializer;

	}



	bool _MeshToMeshphy(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {



		auto findSelectItems = [](std::multimap<uint32, Name>& mapPhyMesh, const Name& meshName) {

			std::string meshNameString = meshName.c_str();

			_CheckFilePath(meshNameString, ".mesh", "");

			Name newMeshName(meshNameString.c_str());

			//try {

			//	MeshPtr res = MeshMgr::instance()->getByName(newMeshName);

			//	if (res.isNull() || !res->isLoaded())

			//	{

			//		res = MeshMgr::instance()->load(newMeshName);

			//	}

			//	if (res.isNull() || !res->isLoaded())

			//	{

			//		return;

			//	}

			//	res->getPhyMeshName(mapPhyMesh);

			//}

			//catch (...) {

			//	LogManager::instance()->logMessage("-error-\t" + newMeshName.toString() + " resource load faild!");

			//}

			mapPhyMesh.insert(std::make_pair(0, newMeshName));

		};

		auto MeshNameToMeshphy = [&](const std::pair<uint32, Name>& srcPhyMesh, bool _IsCover, PhysXMeshVersion version) -> bool {

			std::string phyMeshName = srcPhyMesh.second.toString() + "phy";

			_CheckFilePath(phyMeshName, ".meshphy", "", Name("Physics"));

			std::replace(phyMeshName.begin(), phyMeshName.end(), '/', '_');

			phyMeshName.insert(0, "collision/mesh/");

			Name dstPhyMesh = Name(phyMeshName);

			if (_IsCover || !Root::instance()->TestResExist(dstPhyMesh.c_str(), false, "Physics")) {

				try {

					MeshPtr phyMesh = MeshMgr::instance()->getByName(srcPhyMesh.second);

					if (phyMesh.isNull() || !phyMesh->isLoaded())

					{

						phyMesh = MeshMgr::instance()->load(srcPhyMesh.second);

					}

					if (phyMesh.isNull() || !phyMesh->isLoaded())

					{

						return false;

					}

					PhysXMeshSerializer& serializer = _GetPhysXMeshSerializer();

					MeshToPhysXMeshData toData;

					toData.pMesh = phyMesh;

					toData.surfaceType = srcPhyMesh.first;

					ExportPhysXResType type = serializer.exportPhysXMesh(toData, dstPhyMesh.toString(), version);

					if (type != ExportPhysXResType::eEPRT_SUCCESS) {

						return false;

					}

				}

				catch (...)

				{

					LogManager::instance()->logMessage("-error-\t" + filePath.toString() + " resource load faild!");

					return false;

				}

			}

			return true;

		};



		std::multimap<uint32, Name> selectItems;

		findSelectItems(selectItems, filePath);

		if (selectItems.empty()) {

			return false;

		}

		bool res = true;

		PhysXMeshVersion version = (PhysXMeshVersion)_this->mVersionVector[_this->mVersion].second;

		for (auto& iter : selectItems) {

			res = MeshNameToMeshphy(iter, _this->mIsCover, version) && res;

		}

		if (!res) {

			LogManager::instance()->logMessage("-MeshToMeshphyError-\t" + filePath.toString() + " resource generation failed!");

		}

		return res;

	}



	bool _MeshphyToCooked(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {



		std::string filePathStr = filePath.c_str();

		_CheckFilePath(filePathStr, ".meshphy", "", Name("Physics"));

		Name newFilePath = Name(filePathStr);

		bool resb = true;

		PhysXMeshVersion version = (PhysXMeshVersion)_this->mVersionVector[_this->mVersion].second;

		try {

			PhysXTriangleMeshResourcePtr res = PhysXTriangleMeshResourceManager::instance()->createEmptyResource(newFilePath);

			if (_this->mIsCover || !Root::instance()->TestResExist(res->getCookedPath().c_str(), false, "Physics")) {

				DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(newFilePath.c_str(), false, "Physics"));

				if (!pDataStream.isNull())

				{

					_GetPhysXMeshSerializer().importPhysXMesh(res.get(), pDataStream);

					pDataStream->close();

				}



				if (res->m_mesh)

				{

					std::string path = res->getCookedPath();

					DataStreamPtr stream(Root::instance()->CreateDataStream(path.c_str(),"Physics"));

					if (stream.isNull()) {

						LogManager::instance()->logMessage("Cooked File create failed!");

					}

					else {

						ExportPhysXResType type = _GetPhysXMeshSerializer().exportPhysXMesh_Cooked(res.get(), stream, version);

						stream->close();

						if (type != ExportPhysXResType::eEPRT_SUCCESS) {

							resb = false;

						}

					}

				}

				else {

					LogManager::instance()->logMessage("-MeshphyToCookedError-\t" + filePathStr + " resource generation failed!");

					resb = false;

				}

			}

		}

		catch (...) {

			LogManager::instance()->logMessage("-MeshphyToCookedError-\t" + filePathStr + " resource generation failed!");

			resb = false;

		}





		try {

			PhysXConvexMeshResourcePtr res = PhysXConvexMeshResourceManager::instance()->createEmptyResource(newFilePath);

			if (_this->mIsCover || !Root::instance()->TestResExist(res->getCookedPath().c_str(), false, "Physics")) {



				DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(newFilePath.c_str(), false, "Physics"));

				if (!pDataStream.isNull())

				{

					_GetPhysXMeshSerializer().importPhysXMesh(res.get(), pDataStream);

					pDataStream->close();

				}



				if (res->m_mesh)

				{

					std::string path = res->getCookedPath();

					DataStreamPtr stream(Root::instance()->CreateDataStream(path.c_str(),"Physics"));

					if (stream.isNull()) {

						LogManager::instance()->logMessage("Cooked File create failed!");

					}

					else {

						ExportPhysXResType type = _GetPhysXMeshSerializer().exportPhysXMesh_Cooked(res.get(), stream, version);

						stream->close();

						if (type != ExportPhysXResType::eEPRT_SUCCESS) {

							resb = false;

						}

					}

				}

				else {

					LogManager::instance()->logMessage("-MeshphyToConvexCookedError-\t" + filePathStr + " resource generation failed!");

					resb = false;

				}



			}

		}

		catch (...) {

			LogManager::instance()->logMessage("-MeshphyToConvexCookedError-\t" + filePathStr + " resource generation failed!");

			resb = false;

		}



		return resb;

	}



	//bool _MeshphyToMesh(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {

	//

	//	std::string filePathStr = filePath.c_str();

	//	_CheckFilePath(filePathStr, ".meshphy", "", Name("Physics"));

	//	Name newFilePath = Name(filePathStr);

	//	bool resb = true;

	//	

	//	try {

	//		PhysXConvexMeshResourcePtr res = PhysXConvexMeshResourceManager::instance()->createEmptyResource(newFilePath);

	//		if (_this->mIsCover || !Root::instance()->TestResExist(res->getCookedPath().c_str(), false, "Physics")) {

	//

	//			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(newFilePath.c_str(), false, "Physics));

	//			if (!pDataStream.isNull())

	//			{

	//				_GetPhysXMeshSerializer().importPhysXMesh(res.get(), pDataStream);

	//				pDataStream->close();

	//			}

	//

	//			if (res->m_mesh)

	//			{

	//				

	//				std::vector<Vector3> vb;

	//				std::vector<uint16> ib;

	//				res->m_mesh->getVertexsAndIndices(vb, ib);

	//

	//				String newFilePathStr = newFilePath.toString();

	//				

	//				auto slashPos = newFilePathStr.find_last_of("/\\");

	//				if (slashPos != newFilePathStr.npos) {

	//					newFilePathStr = "model\\collision\\" + newFilePathStr.substr(slashPos + 1);

	//				}

	//				newFilePathStr += ".convex.mesh";

	//

	//				MeshPtr pMesh = MeshMgr::instance()->createResource(Name(newFilePathStr.c_str()), true);

	//					

	//				SubMesh* pSubMesh = pMesh->createSubMesh();

	//

	//				{

	//					

	//					//pSubMesh->setMaterialName(Name("model\\helperobj\\cylinder.material"));

	//					pSubMesh->createMaterialPtr();

	//					auto& ptr = pSubMesh->getMaterialPtr();

	//					ptr->m_matInfo.shadername = "BaseTrans";

	//					//ptr->m_matInfo.shadername = "BaseWhiteNoLight";

	//

	//					unsigned int size = 0;

	//

	//					ptr->m_desc.IADesc.ElementArray[0].nStream = 0;

	//					ptr->m_desc.IADesc.ElementArray[0].eType = ECHO_FLOAT3;

	//					ptr->m_desc.IADesc.ElementArray[0].Semantics = ECHO_POSITION0;

	//					ptr->m_desc.IADesc.ElementArray[0].nOffset = size;

	//

	//					size += 3 * sizeof(float);

	//

	//					//ptr->m_desc.IADesc.ElementArray[1].nStream = 0;

	//					//ptr->m_desc.IADesc.ElementArray[1].eType = ECHO_FLOAT4;

	//					//ptr->m_desc.IADesc.ElementArray[1].Semantics = ECHO_COLOR0;

	//					//ptr->m_desc.IADesc.ElementArray[1].nOffset = size;

	//					//

	//					//size += 4 * sizeof(float);

	//

	//					//ptr->m_matInfo.genmask.append("%TRANSPARENT");

	//

	//					ptr->m_desc.IADesc.nElementCount = 1;

	//					ptr->m_desc.IADesc.nStreamCount = 1;

	//					ptr->m_desc.IADesc.StreamArray[0].nStride = size;

	//

	//

	//					memcpy(&pSubMesh->m_desc.IADesc, &(ptr->m_desc.IADesc), sizeof(pSubMesh->m_desc.IADesc));

	//				}

	//				{

	//

	//					int indexCount = (int)ib.size();

	//					MemoryDataStreamPtr pIndexData(new MemoryDataStream(indexCount * sizeof(Echo::uint16), true));

	//					uint16* pIndexView = (uint16*)pIndexData->getCurrentPtr();

	//

	//					pSubMesh->getIndexData()->setCpuStreamData(pIndexData->getCurrentPtr(), pIndexData);

	//					pSubMesh->getIndexData()->m_indexCount = (unsigned int)indexCount;

	//

	//					for (uint16 flt : ib)

	//						*(pIndexView++) = flt;

	//				}

	//				{

	//						

	//					int chunkSize = (int)pSubMesh->m_desc.IADesc.StreamArray[0].nStride;

	//					int vertexCount = (int)vb.size();

	//					MemoryDataStreamPtr pVertexData(new Echo::MemoryDataStream(chunkSize * vertexCount, true));

	//					float* pVertexView = (float*)pVertexData->getCurrentPtr();

	//

	//					pSubMesh->getVertexData()->addCpuStreamData(pVertexData->getCurrentPtr(), pVertexData, 0, (unsigned short)chunkSize);

	//					pSubMesh->getVertexData()->m_vertexCount = (unsigned int)vertexCount;

	//

	//					for (uint16 i = 0; i < vertexCount; ++i) {

	//						*(pVertexView++) = vb[i].x;

	//						*(pVertexView++) = vb[i].y;

	//						*(pVertexView++) = vb[i].z;

	//					}

	//

	//				}

	//				{

	//					pSubMesh->calcAABB(vb);

	//					pMesh->setAABB(pSubMesh->getBoundingBox().getMinimum(), pSubMesh->getBoundingBox().getMaximum());;

	//				}

	//				pMesh->exportToBinary(pMesh->getName().c_str(), true, false, false, false);

	//			}

	//			else {

	//				LogManager::instance()->logMessage("-MeshphyToConvexCookedError-\t" + filePathStr + " resource generation failed!");

	//				resb = false;

	//			}

	//

	//		}

	//	}

	//	catch (...) {

	//		LogManager::instance()->logMessage("-MeshphyToConvexCookedError-\t" + filePathStr + " resource generation failed!");

	//		resb = false;

	//	}

	//

	//	return resb;

	//}



	bool _MeshphyToConvexMesh(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {



		std::string filePathStr = filePath.c_str();

		_CheckFilePath(filePathStr, ".meshphy", "", Name("Physics"));

		Name newFilePath = Name(filePathStr);

		bool resb = true;

		

		String path = newFilePath.toString();

		{

			auto slashPos = path.find_last_of("/\\");

			if (slashPos != path.npos) {

				path = "model\\collision\\" + path.substr(slashPos + 1);

			}

			path += ".convex.mesh";

		}



		if (_this->mIsCover || !Root::instance()->TestResExist(path.c_str())) {



			try {

				PhysXConvexMeshResourcePtr res = PhysXConvexMeshResourceManager::instance()->getByName(newFilePath);

				if (res.isNull() || !res->isLoaded())

				{

					res = PhysXConvexMeshResourceManager::instance()->load(newFilePath);

				}

				if (!res.isNull() && res->isLoaded() && res->m_mesh)

				{

					resb = res->exportToMesh(path.c_str());



				}

				else {

					LogManager::instance()->logMessage("-MeshphyToConvexCookedError-\t" + filePathStr + " resource generation failed!");

					resb = false;

				}

			}

			catch (...) {

				LogManager::instance()->logMessage("-MeshphyToConvexCookedError-\t" + filePathStr + " resource generation failed!");

				resb = false;

			}



		}



		return resb;

	}



	bool _ActorToActorPhy(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {



		std::string filePathStr = filePath.c_str();

		_CheckFilePath(filePathStr, ".act", "", Name("Actor"));

		Name newFilePath = Name(filePathStr);



		ActorInfoPtr actInfo = ActorInfoManager::instance()->loadInfo(newFilePath);

		if (actInfo.isNull()) {

			return false;

		}



		JSNODE actorphy = cJSON_CreateObject();

		if (EchoPhysicsManager::ExportStaticPhysicsJson(actInfo, actorphy)) {



			char* phyout = cJSON_Print(actorphy);

			cJSON_Delete(actorphy);



			if (0 == phyout) {

				return false;

			}



			String phyActorName = filePathStr + "phy";

			//路径拼出文件名

			std::replace(phyActorName.begin(), phyActorName.end(), '\\', '/');

			std::replace(phyActorName.begin(), phyActorName.end(), '/', '_');



			phyActorName.insert(0, "collision/actor/");



			Echo::DataStreamPtr pStream(Echo::Root::instance()->CreateDataStream(phyActorName.c_str(), "Physics"));

			if (pStream.isNull())

			{

				free(phyout);

				return false;

			}

			else {

				pStream->writeData(phyout, strlen(phyout));

				pStream->close();

				free(phyout);

				return true;

			}

		}

		else {

			cJSON_Delete(actorphy);

			return false;

		}

	}



	String _SPTGenPhysXTest(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {



		std::string filePathStr = filePath.c_str();

		_CheckFilePath(filePathStr, ".terrain", "");

		size_t pos = filePathStr.find("biome_terrain");

		filePathStr = filePathStr.substr(pos);

		String result = filePathStr + " : ";

		String terrainPath = filePathStr;

		ProceduralSphere* pTerrain = new ProceduralSphere();

		pTerrain->init(terrainPath);

		

		if (!pTerrain->m_terrainData.isNull()) {

			switch (_this->mVersion) {

			case 0: {



				std::vector<int> leavesIndexs = pTerrain->getLeavesIndex();

				int size = (int)leavesIndexs.size();

				std::vector<Vector3> vbVec;

				std::vector<uint16> ibVec;

				ibVec = SphericalTerrain::generateIndexBufferPhysX();

				PhysXTriangleMeshResourcePtr resPtr = PhysXTriangleMeshResourceManager::instance()->createEmptyResource(Name::g_cEmptyName);

				std::chrono::steady_clock::time_point tickBegin = std::chrono::steady_clock::now();

				vbVec = pTerrain->generateVertexBufferPhysX(leavesIndexs[0]);

				resPtr->setMeshVBIB(vbVec, ibVec);

				std::chrono::nanoseconds _time = std::chrono::steady_clock::now() - tickBegin;

				double unitTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(_time).count()) * 0.001;

				double allTime = unitTime * (double)size;

				result += std::to_string(unitTime) + "s * " + std::to_string(size) + " = ";

				result += std::to_string(allTime) + "s.";

				break;

			}

			case 1: {



				std::vector<int> leavesIndexs = pTerrain->getLeavesIndex();



				std::vector<Vector3> vbVec;

				std::vector<uint16> ibVec;

				int testNumber = 5;

				std::vector<int> indexs(testNumber);

				int size = (int)leavesIndexs.size();

				for (int i = 0; i != testNumber; ++i) {

					indexs[i] = (int)(Math::UnitRandom() * (size - 1));

				}

				ibVec = SphericalTerrain::generateIndexBufferPhysX();

				PhysXTriangleMeshResourcePtr resPtr = PhysXTriangleMeshResourceManager::instance()->createEmptyResource(Name::g_cEmptyName);

				std::chrono::steady_clock::time_point tickBegin = std::chrono::steady_clock::now();

				for (int index : indexs) {

					vbVec = pTerrain->generateVertexBufferPhysX(leavesIndexs[index]);

					resPtr->setMeshVBIB(vbVec, ibVec);

				}

				std::chrono::nanoseconds _time = std::chrono::steady_clock::now() - tickBegin;

				double allTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(_time).count()) * size * 0.001 / testNumber;

				double unitTime = allTime / (double)size;

				result += std::to_string(unitTime) + "s * " + std::to_string(size) + " = ";

				result += std::to_string(allTime) + "s.";

				break;

			}

			case 2: {



				std::vector<int> leavesIndexs = pTerrain->getLeavesIndex();

				int size = (int)leavesIndexs.size();





				std::vector<Vector3> vbVec;

				std::vector<uint16> ibVec;

				ibVec = SphericalTerrain::generateIndexBufferPhysX();



				PhysXTriangleMeshResourcePtr resPtr = PhysXTriangleMeshResourceManager::instance()->createEmptyResource(Name::g_cEmptyName);

				std::chrono::steady_clock::time_point tickBegin = std::chrono::steady_clock::now();

				for (int index : leavesIndexs) {

					vbVec = pTerrain->generateVertexBufferPhysX(index);

					resPtr->setMeshVBIB(vbVec, ibVec);

				}

				std::chrono::nanoseconds _time = std::chrono::steady_clock::now() - tickBegin;



				double allTime = (double)(std::chrono::duration_cast<std::chrono::seconds>(_time).count());

				double unitTime = allTime / (double)size;



				result += std::to_string(unitTime) + "s * " + std::to_string(size) + " = ";

				result += std::to_string(allTime) + "s.";

				break;

			}

			default: {

				result += "Error.";

				break;

			}

			}

		}

		else {

			result += "Error.";

		}

		



		SAFE_DELETE(pTerrain);



		return result;

	}



	bool _SPTGenPhysXVB(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {

		std::filesystem::path serverPath = Root::instance()->getRootResourcePath();

		serverPath = serverPath.parent_path().parent_path().string() + "\\server";

		std::filesystem::create_directories(serverPath);

		std::string filePathStr = filePath.c_str();

		std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');

		_CheckFilePath(filePathStr, ".terrain", "");

		size_t pos = filePathStr.find("biome_terrain");

		filePathStr = filePathStr.substr(pos);

		return SphericalTerrainComponent::ExportForServer(filePathStr, _this->mVersion, serverPath.string() + "\\", time(NULL));

	}



	bool _SPTGenPcgServer(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {

		std::filesystem::path serverPath = Root::instance()->getRootResourcePath();

		serverPath = serverPath.parent_path().parent_path().string() + "\\server\\biome_terrain";

		std::filesystem::create_directories(serverPath);

		std::string filePathStr = filePath.c_str();

		std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');

		_CheckFilePath(filePathStr, ".terrain", "");

		size_t pos = filePathStr.find("biome_terrain");

		filePathStr = filePathStr.substr(pos);

		return SphericalTerrainComponent::ExportPCGForServer(filePathStr);

	}



	bool _SPTGenPcgDiagram(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {

		std::string filePathStr = filePath.c_str();

		std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');

		_CheckFilePath(filePathStr, ".terrain", "");

		size_t pos = filePathStr.find("biome_terrain");

		filePathStr = filePathStr.substr(pos);

		//return SphericalTerrainComponent::ExportPCGForServer(filePathStr);

		// TODO

		return SphericalTerrainComponent::ExportPCGObjectsBounds(filePathStr);

	}



	bool _SPTCollectData(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {

		set<String>::type resSet;

		String outputPath;

		bool resb = true;

		if (filePath.empty()) {

			SphericalTerrainData::CollectStaticResources(resSet);

			outputPath = "biome_terrain/StaticResource.terrainResourceList";

		}

		else {

			std::string filePathStr = filePath.c_str();

			std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');

			size_t pos = filePathStr.find("biome_terrain");

			if (pos == std::string::npos)

				return false;

			String terrainPath = filePathStr.substr(pos);

			SphericalTerrainData stData;

			resb = SphericalTerrainResource::loadSphericalTerrain(terrainPath, stData, false);

			stData.CollectResources(resSet);

			outputPath = terrainPath + "ResourceList";

		}

		for (auto& res : resSet) {

			if (res.empty()) continue;

			LogManager::instance()->logMessage(res);

		}

		if (DataStreamPtr stream(Root::instance()->CreateDataStream(outputPath.c_str())); !stream.isNull())

		{

			for (auto& res : resSet) {

				if (res.empty()) continue;

				String line = res + "\n";

				stream->writeData(line.data(), line.length());

			}

			stream->close();

		}

		return resb;

	}



	std::vector<float> _GetSampleValue(int sampleNumberAdd1) {

		int sampleNumber = sampleNumberAdd1 - 1;

		float sampleInterval = 2.0f / (float)sampleNumber;

		uint32 halfSampleNumber = sampleNumber / 2;

		std::vector<float> sampleValues(sampleNumberAdd1);

		//for (uint32 i = 0; i != sampleNumberAdd1; ++i) {

		//	if (i == halfSampleNumber) {

		//		sampleValues[i] = 0.0f;

		//	}

		//	else if (i < halfSampleNumber) {

		//		sampleValues[i] = (float)i * sampleInterval - 1.0f;

		//	}

		//	else {

		//		sampleValues[i] = -sampleValues[sampleNumber - i];

		//	}

		//}

		for (uint32 i = 0; i != sampleNumberAdd1; ++i) {

			sampleValues[i] = (float)i * sampleInterval - 1.0f;

		}

		return sampleValues;

	}



	struct TestGeomMesh {

	public:

		TestGeomMesh() {}

		struct Side {

			uint16 v1 = 0;

			uint16 v2 = 0;

			int triangle1 = -1;

			int triangle2 = -1;

		};

		std::map<std::pair<uint16, uint16>, int> sideMap;

		std::vector<Side> sides;

		std::vector<Vector3> triangleNormals;

		EchoTriangleMeshGeometry triangleGeom;

		std::vector<std::pair<EchoCapsuleGeometry, Transform>> capsuleGeoms;

		EchoTriangleMeshGeometryDEL newTriangleGeom;

		bool bSuccess = false;

		float sqrtAreaRatio = 1.0f;

		float minVertexScale = FLT_MAX;

		void addSide(uint16 v1, uint16 v2, int triangle) {

			std::pair<uint16, uint16> pp;

			if (v1 < v2) {

				pp.first = v1;

				pp.second = v2;

			}

			else {

				pp.first = v2;

				pp.second = v1;

			}

			auto iter = sideMap.find(pp);

			if (iter == sideMap.end()) {

				sideMap[pp] = (int)sides.size();

				auto& side = sides.emplace_back();

				side.v1 = pp.first;

				side.v2 = pp.second;

				side.triangle1 = triangle;

				side.triangle2 = -1;

			}

			else {

				sides[iter->second].triangle2 = triangle;

			}

		}

		void Create(EchoTriangleMeshGeometry& mTriangle, float sphereRadius) {

			triangleGeom = mTriangle;

			if(sphereRadius != 0.0f)

			{

				std::vector<Vector3> vbVec;

				std::vector<uint16> ibVec;

				mTriangle.triangle->getVertexsAndIndices(vbVec, ibVec);



				int nbVertex = (int)vbVec.size();

				for (int i = 0; i < nbVertex; ++i) {

					float vertexLength = vbVec[i].length();

					float vertexScale = (sphereRadius / vertexLength) + 1.0f;

					if (vertexScale < minVertexScale) {

						minVertexScale = vertexScale;

					}

				}



				float triangleAreaSum = 0.0f;



				int nbTriangle = mTriangle.triangle->getNbTriangles();

				triangleNormals.resize(nbTriangle);

				sides.reserve(nbTriangle * 3);

				for (int i = 0; i < nbTriangle; ++i)

				{

					addSide(ibVec[i * 3 + 0], ibVec[i * 3 + 1], i);

					addSide(ibVec[i * 3 + 1], ibVec[i * 3 + 2], i);

					addSide(ibVec[i * 3 + 0], ibVec[i * 3 + 2], i);

					triangleNormals[i] = Vector3::VCross(vbVec[ibVec[i * 3 + 1]] - vbVec[ibVec[i * 3]], vbVec[ibVec[i * 3 + 2]] - vbVec[ibVec[i * 3]]).GetNormalized();

					triangleAreaSum += Vector3::VCross(vbVec[ibVec[i * 3 + 1]] - vbVec[ibVec[i * 3]], vbVec[ibVec[i * 3 + 2]] - vbVec[ibVec[i * 3]]).length() / 2.0f;

				}



				int nbSide = (int)sides.size();

				capsuleGeoms.reserve(nbSide);

				for (int i = 0; i < nbSide; ++i) {

					auto& side = sides[i];

					float triDot = triangleNormals[side.triangle1].Dot(triangleNormals[side.triangle2]);

					if (Math::FloatEqual(triDot, 1.0f, 0.01f)) {

						continue;

					}

					auto& capsule = capsuleGeoms.emplace_back();

					capsule.first.radius = sphereRadius;

					Vector3 capsuleDir = vbVec[side.v1] - vbVec[side.v2];

					capsule.first.halfHeight = capsuleDir.length() * 0.5f;

					capsule.first.upDirection = EchoCapsuleGeometry::eX;

					capsule.second.translation = vbVec[side.v2] + capsuleDir * 0.5f;

					capsule.second.rotation.RotationAxisToAxis(Vector3::UNIT_X, capsuleDir.GetNormalized());



					float capsuleArea = capsule.first.halfHeight * 2.0f * sphereRadius * Math::ACos(triDot).valueRadians();

					triangleAreaSum += capsuleArea;

				}



				std::vector<Vector3> newVbVec;

				std::vector<uint16> newIbVec;



				for (int i = 0; i < nbTriangle; ++i)

				{

					Vector3 offset = triangleNormals[i] * sphereRadius;

					uint16 startIndex = (uint16)newVbVec.size();

					newVbVec.push_back(vbVec[ibVec[i * 3 + 0]] + offset);

					newVbVec.push_back(vbVec[ibVec[i * 3 + 1]] + offset);

					newVbVec.push_back(vbVec[ibVec[i * 3 + 2]] + offset);

					newIbVec.push_back(startIndex);

					newIbVec.push_back(startIndex+1);

					newIbVec.push_back(startIndex+2);

				}

				newTriangleGeom.scale = Vector3::UNIT_SCALE;

				newTriangleGeom.triangle = PhysXSystem::instance()->MeshDataToPxTriangleMesh(newVbVec, newIbVec);

				bSuccess = true;



				sqrtAreaRatio = std::sqrtf(triangleAreaSum / ProceduralSphere::unitSphereArea);

			}

			else {

				minVertexScale = 1.0f;

				sqrtAreaRatio = 1.0f;

				{

					float triangleAreaSum = 0.0f;

					std::vector<Vector3> vbVec;

					std::vector<uint16> ibVec;

					mTriangle.triangle->getVertexsAndIndices(vbVec, ibVec);

					int nbTriangle = mTriangle.triangle->getNbTriangles();

					for (int i = 0; i < nbTriangle; ++i)

					{

						triangleAreaSum += Vector3::VCross(vbVec[ibVec[i * 3 + 1]] - vbVec[ibVec[i * 3]], vbVec[ibVec[i * 3 + 2]] - vbVec[ibVec[i * 3]]).length() / 2.0f;

					}

					sqrtAreaRatio = std::sqrtf(triangleAreaSum / ProceduralSphere::unitSphereArea);

				}

			}

		}

		bool raycast(const Vector3& direction, float& _height) {

			EchoLocationHit hit;

			if (bSuccess) {

				float minDis = FLT_MAX;

				int nbCapsule = (int)capsuleGeoms.size();

				Vector3 origin = direction * 2.0f;

				for (int i = 0; i != nbCapsule; ++i) {

					if (EchoGeometryQuery::Raycast(origin, -direction, 2.0f, capsuleGeoms[i].first, capsuleGeoms[i].second, hit)) {

						if (hit.distance < minDis && hit.distance < 2.0f) {

							minDis = hit.distance;

						}

					}

				}

				if (EchoGeometryQuery::Raycast(Vector3::ZERO, direction, 2.0f, newTriangleGeom, Transform::IDENTITY, hit, (EchoHitFlag)((uint16)EchoHitFlag::eDEFAULT1 | (uint16)EchoHitFlag::eMESH_BOTH_SIDES))) {

					if (hit.distance < 2.0f) {

						float newDis = 2.0f - hit.distance;

						if (newDis < minDis) {

							minDis = newDis;

						}

					}

				}

				if (minDis < 2.0f) {

					_height = 2.0f - minDis;

					return true;

				}

				else {

					return false;

				}

			}

			else {

				Vector3 origin = Vector3::ZERO;

				if (EchoGeometryQuery::Raycast(origin, direction, 2.0f, triangleGeom, Transform::IDENTITY, hit, (EchoHitFlag)((uint16)EchoHitFlag::eDEFAULT1 | (uint16)EchoHitFlag::eMESH_BOTH_SIDES))) {

					_height = hit.distance;

					return true;

				}

				else {



					return false;

				}

			}

		}

	};



	void _SampleHeightMaps(TestGeomMesh& mTriangle, std::array<std::vector<float>, 6>& heightmaps, int sampleNumber) {

		uint32 sampleNumberAdd1 = sampleNumber + 1;

		int heightmapSize = sampleNumberAdd1 * sampleNumberAdd1;

		std::vector<float> sampleValues = _GetSampleValue(sampleNumberAdd1);

		Transform trans = Transform::IDENTITY;

		for (uint8 face = 0; face < 6; ++face) {

			heightmaps[face].resize(heightmapSize);

			for (int i = 0; i != sampleNumberAdd1; ++i) {

				for (int j = 0; j != sampleNumberAdd1; ++j) {

					int heightmapIndex = i * sampleNumberAdd1 + j;

					Vector3 normal = SphericalTerrainQuadTreeNode::MapToSphere(face, Vector2(sampleValues[i], sampleValues[j]));

					float height;

					if (mTriangle.raycast(normal.GetNormalized(), height)) {

						heightmaps[face][heightmapIndex] = height;

					}

					else {

						heightmaps[face][heightmapIndex] = 1.0f;

					}

				}

			}

		}

	};



	void _SampleBorderMaps(PolyhedronQuery* pQuery, std::array<std::vector<float>, 6>& heightmaps, std::array<std::vector<float>, 6>& bordermaps, int sampleNumber) {

		uint32 sampleNumberAdd1 = sampleNumber + 1;

		int heightmapSize = sampleNumberAdd1 * sampleNumberAdd1;

		std::vector<float> sampleValues = _GetSampleValue(sampleNumberAdd1);

		Transform trans = Transform::IDENTITY;

		for (uint8 face = 0; face < 6; ++face) {

			bordermaps[face].resize(heightmapSize);

			for (int i = 0; i != sampleNumberAdd1; ++i) {

				for (int j = 0; j != sampleNumberAdd1; ++j) {

					int heightmapIndex = i * sampleNumberAdd1 + j;

					Vector3 point = SphericalTerrainQuadTreeNode::MapToSphere(face, Vector2(sampleValues[i], sampleValues[j])) * heightmaps[face][heightmapIndex];

					bordermaps[face][heightmapIndex] = pQuery->getBorder(point);

				}

			}

		}

	};

	

	bool _SPTGeometryHeightMapSample(Name& filePath, const PhysicsToolsUI::PhysicsToolStruct* _this) {

		std::string filePathStr = filePath.c_str();

		std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');

		size_t pos = filePathStr.find("collision/mesh/");

		if (pos == std::string::npos)

			return false;

		String meshphyPath = filePathStr.substr(pos);



		Name newFilePath = Name(meshphyPath);



		pos = meshphyPath.find_last_of("/\\");

		if (pos == std::string::npos)

			return false;

		meshphyPath = meshphyPath.substr(pos);

		pos = meshphyPath.find(".meshphy");

		if (pos == std::string::npos)

			return false;





		bool resb = true;

		uint32 sampleNumber = _this->mVersionVector[_this->mVersion].second;



		String heightMapPath = "echo/biome_terrain/geometryType/" + meshphyPath.substr(0, pos) + "_" + std::to_string(sampleNumber) + ".gemp";

		String planetGeometryPath = "biome_terrain/geometryType/" + meshphyPath.substr(0, pos) + ".geometry";

		String gravityPath = "echo/biome_terrain/geometryType/" + meshphyPath.substr(0, pos) + ".gravity";

		String borderPath = "echo/biome_terrain/geometryType/" + meshphyPath.substr(0, pos) + ".border";



		PhysXTriangleMeshResourcePtr mTriangleMesh;

		PhysXConvexMeshResourcePtr mConvexMesh;

		EchoTriangleMeshGeometry mTriangle;

		EchoConvexMeshGeometry mConvex;



		mTriangleMesh = PhysXTriangleMeshResourceManager::instance()->load(newFilePath);

		mConvexMesh = PhysXConvexMeshResourceManager::instance()->load(newFilePath);



		if (mConvexMesh.isNull() || mConvexMesh->m_mesh == nullptr || mTriangleMesh.isNull() || mTriangleMesh->m_mesh == nullptr) return false;



		mTriangle.triangle = mTriangleMesh->m_mesh;

		mConvex.convex = mConvexMesh->m_mesh;



		if (mConvex.convex == nullptr) {

			return false;

		}

		if (mTriangle.triangle == nullptr) {

			return false;

		}



		std::vector<Vector4> convexPlanes;

		{

			uint32 nbPolygons = mConvex.convex->getNbPolygons();

			const uint8* indexBuffer = mConvex.convex->getIndexBuffer();

			const Vector3* pVertex = (const Vector3*)mConvex.convex->getVertices();

			convexPlanes.resize(nbPolygons);

			for (uint32 polIndex = 0; polIndex != nbPolygons; ++polIndex) {

				PhysXHullPolygon polygon;

				mConvex.convex->getPolygonData(polIndex, polygon);

				if (polygon.mPlane[3] > 0.0f) return false;

				convexPlanes[polIndex] = Vector4(polygon.mPlane[0], polygon.mPlane[1], polygon.mPlane[2], polygon.mPlane[3]);

			}

		}



		TestGeomMesh _testGeom;

		_testGeom.Create(mTriangle, _this->mCustomF0);



		float sqrtAreaRatio = _testGeom.sqrtAreaRatio;

		float minVertexScale = _testGeom.minVertexScale;





		uint32 planeSize = (uint32)convexPlanes.size();

		for (uint32 planeIndex = 0; planeIndex != planeSize; ++planeIndex) {

			convexPlanes[planeIndex].w *= minVertexScale;

		}



		std::array<std::vector<float>, 6> heightmaps;

		uint32 heightMapSize = sampleNumber + 1;

		_SampleHeightMaps(_testGeom, heightmaps, sampleNumber);



		bool bResult = false;

		if (DataStreamPtr stream(Root::instance()->CreateDataStream(heightMapPath.c_str())); !stream.isNull())

		{

			uint32 version = 2;



			stream->writeData(&version, 1);

			stream->writeData(&heightMapSize, 1);

			stream->writeData(&planeSize, 1);

			stream->writeData(&sqrtAreaRatio, 1);



			if (planeSize != 0) {

				stream->write(convexPlanes.data(), planeSize * sizeof(Vector4));

			}

			if (heightMapSize != 0) {

				for (int face = 0; face < 6; ++face)

				{

					stream->write(heightmaps[face].data(), heightmaps[face].size() * sizeof(float));

				}

			}

			stream->close();

			bResult = true;

		}

		if (DataStreamPtr stream(Root::instance()->CreateDataStream(planetGeometryPath.c_str(), "Physics")); !stream.isNull())

		{

			const uint32 PLANET_GEOMETRY_HEADER_CHUNK_ID = 'P' | ('T' << 8) | ('G' << 16) | ('Y' << 24);

			uint32 version = 0;



			stream->writeData(&PLANET_GEOMETRY_HEADER_CHUNK_ID, 1);

			stream->writeData(&version, 1);

			stream->writeData(&minVertexScale, 1);



			std::vector<Vector3> _vbVec;

			std::vector<uint16> _ibVec;

			mTriangle.triangle->getVertexsAndIndices(_vbVec, _ibVec);



			uint32 nbVertex = (uint32)_vbVec.size();

			uint32 nbTriangle = (uint32)_ibVec.size();



			stream->writeData(&nbVertex, 1);

			stream->writeData(&nbTriangle, 1);



			stream->write(_vbVec.data(), nbVertex * sizeof(Vector3));

			stream->write(_ibVec.data(), nbTriangle * sizeof(uint16));



			stream->close();

		}

		else {

			bResult = false;

		}

		PolyhedronQuery::Polyhedron polyStruct;

		{

			uint32 nbPolygons = mConvexMesh->m_mesh->getNbPolygons();

			const uint8* indexBuffer = mConvexMesh->m_mesh->getIndexBuffer();

			const Vector3* pVertex = (const Vector3*)mConvexMesh->m_mesh->getVertices();

			uint32 nbVertexs = mConvexMesh->m_mesh->getNbVertices();

			polyStruct.vertexs.resize(nbVertexs);

			std::memcpy(polyStruct.vertexs.data(), pVertex, sizeof(Vector3) * nbVertexs);

			polyStruct.polygons.resize(nbPolygons);

			for (uint32 polIndex = 0; polIndex != nbPolygons; ++polIndex) {

				PhysXHullPolygon pxpolygon;

				mConvexMesh->m_mesh->getPolygonData(polIndex, pxpolygon);

				if (pxpolygon.mPlane[3] > 0.0f) return false;

				auto& polygon = polyStruct.polygons[polIndex];

				polygon.indexs.resize(pxpolygon.mNbVerts);

				const uint8* faceIndex = indexBuffer + pxpolygon.mIndexBase;

				for (int j = 0; j != pxpolygon.mNbVerts; ++j) {

					polygon.indexs[j] = faceIndex[j];

				}

				polygon.normal = Vector3(pxpolygon.mPlane[0], pxpolygon.mPlane[1], pxpolygon.mPlane[2]);

			}

		}

		if (DataStreamPtr stream(Root::instance()->CreateDataStream(gravityPath.c_str())); !stream.isNull())

		{

			uint32 version = 0;

			uint32 vertexSize = (uint32)polyStruct.vertexs.size();

			uint32 polygonSize = (uint32)polyStruct.polygons.size();



			stream->writeData(&version, 1);

			stream->writeData(&vertexSize, 1);

			stream->writeData(&polygonSize, 1);



			stream->writeData(polyStruct.vertexs.data(), vertexSize);



			for (int i = 0; i != polygonSize; ++i) {

				stream->writeData(&polyStruct.polygons[i].normal, 1);

				int indexSize = (int)polyStruct.polygons[i].indexs.size();

				stream->writeData(&indexSize, 1);

				stream->writeData(polyStruct.polygons[i].indexs.data(), indexSize);

			}

			stream->close();

		}

		else {

			bResult = false;

		}

		{

			std::array<std::vector<float>, 6> bordermaps;

			uint32 borderSize = sampleNumber + 1;

			PolyhedronQuery polyQuery(polyStruct);

			_SampleBorderMaps(&polyQuery, heightmaps, bordermaps, sampleNumber);



			if (DataStreamPtr stream(Root::instance()->CreateDataStream(borderPath.c_str())); !stream.isNull())

			{

				stream->writeData(&borderSize, 1);

				if (borderSize != 0) {

					for (int face = 0; face < 6; ++face)

					{

						stream->write(bordermaps[face].data(), bordermaps[face].size() * sizeof(float));

					}

				}

				stream->close();

			}

			else {

				bResult = false;

			}

		}

		return bResult;

	}



	void PhysicsToolsUI::InitGui(int classID)

	{

		mUIType = U8("物理工具界面");

		mUIName = U8("物理工具界面##") + std::to_string(classID);

		ImGui::SetNextWindowPos(mPosition, mPositionType);

		ImGui::SetNextWindowSize(mSize, mSizeType);



		std::string _MeshFolderPath = Root::instance()->getRootResourcePath() + "model";

		std::string _MeshphyFolderPath = Root::instance()->getRootResourcePath("Physics") + "collision\\mesh";

		std::string _TerrainFolderPath = Root::instance()->getRootResourcePath("Physics") + "collision\\terrain";

		std::string _ActorPhyFolderPath = Root::instance()->getRootResourcePath("Actor") + "actors";

		std::string _SPTGenPhysXPath = Root::instance()->getRootResourcePath() + "biome_terrain";

		std::replace(_MeshFolderPath.begin(), _MeshFolderPath.end(), '/', '\\');

		std::replace(_MeshphyFolderPath.begin(), _MeshphyFolderPath.end(), '/', '\\');

		std::replace(_TerrainFolderPath.begin(), _TerrainFolderPath.end(), '/', '\\');

		std::replace(_ActorPhyFolderPath.begin(), _ActorPhyFolderPath.end(), '/', '\\');

		std::replace(_SPTGenPhysXPath.begin(), _SPTGenPhysXPath.end(), '/', '\\');





		String _ConvexMeshExportPath = Root::instance()->getRootResourcePath() + "model\\collision";

		std::replace(_ConvexMeshExportPath.begin(), _ConvexMeshExportPath.end(), '/', '\\');

		if (_access(_ConvexMeshExportPath.c_str(), 0) == -1) {

			CreateDirectoryA(_ConvexMeshExportPath.c_str(), NULL);

		}

		

		mMeshTool.mExtension = "mesh";

		mMeshTool.mFolderPath = _MeshFolderPath;

		mMeshTool.mTaskFunction = _MeshToMeshphy;

		mMeshTool.mVersionVector.clear();

		mMeshTool.mVersionVector.push_back(std::make_pair(Name("LATEST"), (uint32)PHYSXMESH_VERSION_LATEST));

		mMeshTool.mVersionVector.push_back(std::make_pair(Name("1_0"), (uint32)PHYSXMESH_VERSION_1_0));

		mMeshTool.mVersionVector.push_back(std::make_pair(Name("1_1"), (uint32)PHYSXMESH_VERSION_1_1));

		mMeshTool.mVersionVector.push_back(std::make_pair(Name("LEGACY"), (uint32)PHYSXMESH_VERSION_LEGACY));

		mMeshTool.mVersion = 0;



		mMeshphyTool.mExtension = "meshphy";

		mMeshphyTool.mFolderPath = _MeshphyFolderPath;

		mMeshphyTool.mTaskFunction = _MeshphyToCooked;

		mMeshphyTool.mVersionVector.clear();

		mMeshphyTool.mVersionVector.push_back(std::make_pair(Name("LATEST"), (uint32)PHYSXMESH_VERSION_LATEST));

		mMeshphyTool.mVersionVector.push_back(std::make_pair(Name("1_0"), (uint32)PHYSXMESH_VERSION_1_0));

		mMeshphyTool.mVersionVector.push_back(std::make_pair(Name("1_1"), (uint32)PHYSXMESH_VERSION_1_1));

		mMeshphyTool.mVersionVector.push_back(std::make_pair(Name("LEGACY"), (uint32)PHYSXMESH_VERSION_LEGACY));

		mMeshphyTool.mVersion = 0;



		mMeshphyToMeshTool.mExtension = "meshphy";

		mMeshphyToMeshTool.mFolderPath = _MeshphyFolderPath;

		mMeshphyToMeshTool.mTaskFunction = _MeshphyToConvexMesh;

		mMeshphyToMeshTool.mVersionVector.clear();

		mMeshphyToMeshTool.mVersion = 0;



		mActorPhyTool.mExtension = "act";

		mActorPhyTool.mFolderPath = _ActorPhyFolderPath; 

		mActorPhyTool.mTaskFunction = _ActorToActorPhy;

		mActorPhyTool.mVersionVector.clear();

		mActorPhyTool.mVersionVector.push_back(std::make_pair(Name("LATEST"), (uint32)0));

		mActorPhyTool.mVersion = 0;



		mSPTGenPhysXTestTool.mExtension = "terrain";

		mSPTGenPhysXTestTool.mFolderPath = _SPTGenPhysXPath;

		mSPTGenPhysXTestTool.mTaskFunction2 = _SPTGenPhysXTest;

		mSPTGenPhysXTestTool.mVersionVector.clear();

		mSPTGenPhysXTestTool.mVersionVector.push_back(std::make_pair(Name("FastTest"), (uint32)0));

		mSPTGenPhysXTestTool.mVersionVector.push_back(std::make_pair(Name("FastTest2"), (uint32)1));

		mSPTGenPhysXTestTool.mVersionVector.push_back(std::make_pair(Name("RealTest"), (uint32)2));

		mSPTGenPhysXTestTool.mVersion = 1;





		mSPTGenPhysXVBTool.mExtension = "terrain";

		mSPTGenPhysXVBTool.mFolderPath = _SPTGenPhysXPath;

		mSPTGenPhysXVBTool.mTaskFunction = _SPTGenPhysXVB;





		mSPTCollectDataTool.mExtension = "terrain";

		mSPTCollectDataTool.mFolderPath = _SPTGenPhysXPath;

		mSPTCollectDataTool.mTaskFunction = _SPTCollectData;



		mSPTGeomHeightMapSampleTool.mExtension = "meshphy";

		mSPTGeomHeightMapSampleTool.mFolderPath = _MeshphyFolderPath;

		mSPTGeomHeightMapSampleTool.mTaskFunction = _SPTGeometryHeightMapSample;

		mSPTGeomHeightMapSampleTool.mVersionVector.clear();

		mSPTGeomHeightMapSampleTool.mVersionVector.push_back(std::make_pair(Name("256"), (uint32)256));

		mSPTGeomHeightMapSampleTool.mVersionVector.push_back(std::make_pair(Name("512"), (uint32)512));

		mSPTGeomHeightMapSampleTool.mVersionVector.push_back(std::make_pair(Name("1024"), (uint32)1024));

		mSPTGeomHeightMapSampleTool.mVersionVector.push_back(std::make_pair(Name("2048"), (uint32)2048));

		mSPTGeomHeightMapSampleTool.mCustomF0 = 0.06f;

		mSPTGeomHeightMapSampleTool.mUserDataShow = [](PhysicsToolStruct* _this) {

			if (!_this->mFilePathQueue.empty() && !_this->mQueueIsWork) {

				if (ImGui::DragFloat(U8("球面边角长度"), &_this->mCustomF0, 0.01f, 0.01f, 0.5f, "%.2f")) {

					_this->mCustomF0 = Math::Clamp(_this->mCustomF0, 0.00f, 0.5f);

					_this->mCustomF0 = std::floorf(_this->mCustomF0 * 100.0f) / 100.0f;

				}

			}

		};

		mSPTGeomHeightMapSampleTool.mVersion = 1;





		mSPTGenPcgServerTool.mExtension = "terrain";

		mSPTGenPcgServerTool.mFolderPath = _SPTGenPhysXPath;

		mSPTGenPcgServerTool.mTaskFunction = _SPTGenPcgServer;



		mSPTGenPcgDiagramTool.mExtension = "terrain";

		mSPTGenPcgDiagramTool.mFolderPath = _SPTGenPhysXPath;

		mSPTGenPcgDiagramTool.mTaskFunction = _SPTGenPcgDiagram;

		mSPTGenPcgDiagramTool.mUserDataShow = [](PhysicsToolStruct* _this)
		{
			if (!_this->mFilePathQueue.empty() && !_this->mQueueIsWork)
			{
				// 格子尺寸设置
				float len = SphericalTerrainComponent::GetHeatmapDesiredCellLen();
				if (ImGui::DragFloat(U8("热图格子尺寸(米)"), &len, 10.0f, 1.0f, 100000.0f, "%.0f"))
				{
					len = Math::Clamp(len, 1.0f, 100000.0f);
					SphericalTerrainComponent::SetHeatmapDesiredCellLen(std::floorf(len));
				}
				
				// 颜色模式设置
				static const char* colorModeItems[] = { U8("RGB彩虹色"), U8("单色渐变") };
				int currentMode = (int)SphericalTerrainComponent::GetHeatmapColorMode();
				if (ImGui::Combo(U8("热图颜色模式"), &currentMode, colorModeItems, 2))
				{
					SphericalTerrainComponent::SetHeatmapColorMode((SphericalTerrainComponent::HeatmapColorMode)currentMode);
				}
				
				// 如果是单色模式，显示颜色选择器
				if (SphericalTerrainComponent::GetHeatmapColorMode() == SphericalTerrainComponent::HeatmapColorMode::HEATMAP_SINGLE_COLOR)
				{
					ColorValue baseColor = SphericalTerrainComponent::GetHeatmapBaseColor();
					float color[3] = { baseColor.r, baseColor.g, baseColor.b };
					if (ImGui::ColorEdit3(U8("基础颜色"), color))
					{
						SphericalTerrainComponent::SetHeatmapBaseColor(ColorValue(color[0], color[1], color[2], 1.0f));
					}
				}
				
				ImGui::Separator();
				
				// 手动阈值设置
				bool useManual = SphericalTerrainComponent::GetUseManualThresholds();
				if (ImGui::Checkbox(U8("使用手动阈值"), &useManual))
				{
					SphericalTerrainComponent::SetUseManualThresholds(useManual);
				}
				
				if (useManual)
				{
					float maxThreshold = SphericalTerrainComponent::GetManualMaxThreshold();
					if (ImGui::DragFloat(U8("上限阈值"), &maxThreshold, 100.0f, 1.0f, 1000000.0f, "%.0f"))
					{
						SphericalTerrainComponent::SetManualMaxThreshold(maxThreshold);
					}
					
					float minThreshold = SphericalTerrainComponent::GetManualMinThreshold();
					if (ImGui::DragFloat(U8("下限阈值"), &minThreshold, 10.0f, 0.0f, 100000.0f, "%.0f"))
					{
						SphericalTerrainComponent::SetManualMinThreshold(minThreshold);
					}
					
					ImGui::TextWrapped(U8("说明：超过上限阈值的区域显示为最高强度，低于下限阈值的区域显示为黑色"));
				}
			}
		};

	}



	void PhysicsToolsUI::Keyboard() {}



	PhysicsToolsUI::PhysicsToolsUI(int classID, ImGuiContext* ctx)

		: BaseUI(classID, ctx)

		, mPosition(0.f, 0)

		, mPositionType(ImGuiCond_FirstUseEver)

		, mSize(300.f, 300)

		, mSizeType(ImGuiCond_FirstUseEver)

	{

		InitGui(classID);

		IsCreated = true;

	}

	PhysicsToolsUI::~PhysicsToolsUI() {

		IsCreated = false;

	}



	PhysicsToolsUI::PhysicsToolStruct::~PhysicsToolStruct() {}

	

	void PhysicsToolsUI::ShowGui() {

		if (!ImGui::BeginChild(mUIName.c_str())) {

			ImGui::EndChild();

			return;

		}



		if (ImGui::TreeNodeEx("MeshToMeshphy", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mMeshTool.ShowUI();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("MeshphyToCooked", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mMeshphyTool.ShowUI();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("MeshphyToConvexMesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mMeshphyToMeshTool.ShowUI();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("ActorToActorPhy", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mActorPhyTool.ShowUI();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("SPTGenPhysXTest", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			

			mSPTGenPhysXTestTool.ShowUI2();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("SPTCollectResource", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mSPTCollectDataTool.ShowUI();

			if (ImGui::Button(U8("静态资源"), ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth * 0.5f, 0.f))) {

				Name nullPath = Name::g_cEmptyName;

				mSPTCollectDataTool.mTaskFunction(nullPath, &mSPTCollectDataTool);

			}

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("SPTGenServerData", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mSPTGenPhysXVBTool.ShowUI();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("SPTGenHeightMap", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mSPTGeomHeightMapSampleTool.ShowUI();

			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("OAECollectResource", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {



			if (ImGui::Button(U8("收集并显示资源"), ImVec2(ImGui::GetCurrentWindow()->DC.ItemWidth * 0.5f, 0.f))) {

				std::map<OAE::OAEResourceType, std::set<String>> fileMaps;

				OAE::OAESystem::instance()->getMainManager()->collectAllResource(fileMaps);

				for (const auto& iter1 : fileMaps) {

					for (const auto& iter2 : iter1.second) {

						LogManager::instance()->logMessage(iter2.c_str());

					}

				}

			}



			ImGui::TreePop();

		}



		if (ImGui::TreeNodeEx("SPTGenPcgServer", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mSPTGenPcgServerTool.ShowUI();

			ImGui::TreePop();

		}





		if (ImGui::TreeNodeEx("SPTGenPcgDiagram", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

			mSPTGenPcgDiagramTool.ShowUI();

			ImGui::TreePop();

		}



		ImGui::EndChild();

	}



	int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {

		if (uMsg == BFFM_INITIALIZED) {

			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);

		}

		return 0;

	}



	bool _LoadFolder(std::string& folderPath) {

		TCHAR szBuffer[TEXT_BUFFER_MAX_PATH] = { 0 };

		BROWSEINFO bi;

		ZeroMemory(&bi, sizeof(BROWSEINFO));

		bi.hwndOwner = NULL;

		bi.pszDisplayName = szBuffer;

		bi.lpszTitle = _T("Select Folder Directory:");

		bi.ulFlags = BIF_RETURNFSANCESTORS;

		if (!folderPath.empty()) {

			bi.lParam = (LPARAM)((void*)folderPath.c_str());

			bi.lpfn = BrowseCallbackProc;

		}

		LPITEMIDLIST idl = SHBrowseForFolder(&bi);

		if (NULL == idl)

		{

			return false;

		}

		SHGetPathFromIDList(idl, szBuffer);

		folderPath = bi.pszDisplayName;

		return true;

	}

	bool _LoadFile(std::string& folderPath, std::vector<Name>& filePathQueue, const char* extension) {

		OPENFILENAME ofn;

		std::vector<TCHAR> szOpenFileNames(100 * TEXT_BUFFER_MAX_PATH, '\0');

		TCHAR szPath[TEXT_BUFFER_MAX_PATH];

		TCHAR szFileName[TEXT_BUFFER_MAX_PATH];

		TCHAR* p;

		int nLen = 0;

		ZeroMemory(&ofn, sizeof(ofn));

		ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT;

		ofn.lStructSize = sizeof(ofn);

		ofn.lpstrFile = szOpenFileNames.data();

		ofn.nMaxFile = (DWORD)szOpenFileNames.size();

		ofn.lpstrFile[0] = '\0';



		size_t extLen = strlen(extension);

		std::vector<char> filter(10 + 2 * extLen, '\0');

		char* filterPtr = filter.data();

		memcpy(filterPtr, extension, extLen);

		filterPtr += extLen;

		memcpy(filterPtr, " File\0*.", 8);

		filterPtr += 8;

		memcpy(filterPtr, extension, extLen);



		ofn.lpstrFilter = filter.data();



		ofn.lpstrInitialDir = folderPath.c_str();

		if (GetOpenFileName(&ofn)) {

			lstrcpyn(szPath, szOpenFileNames.data(), ofn.nFileOffset);

			szPath[ofn.nFileOffset] = '\0';

			nLen = lstrlen(szPath);

			if (szPath[nLen - 1] != '\\') {

				lstrcat(szPath, TEXT("\\"));

			}

			p = szOpenFileNames.data() + ofn.nFileOffset;

			while (*p) {

				ZeroMemory(szFileName, TEXT_BUFFER_MAX_PATH);

				lstrcat(szFileName, szPath);

				lstrcat(szFileName, p);

				p += lstrlen(p) + 1;

				filePathQueue.push_back(Name(szFileName));

			}

		}

		return true;

	}



	std::string _getHeaderChunkID(Name& extension) {

		static const char* HEADER_CHUNK_ID = "PhysicsTool";

		return std::string(HEADER_CHUNK_ID) + extension.toString();

	}



	bool ShowVersionList(const char* label, uint32& _index, std::vector<std::pair<Name, uint32>>& vec) {

		int index = (int)_index;

		int size = (int)vec.size();

		std::vector<const char*> items(size);

		for (int i = 0; i != size; ++i) {

			items[i] = vec[i].first.c_str();

		}

		if (ImGui::Combo(label, &index, items.data(), size)) {

			_index = index;

			return true;

		}

		return false;

	}



	void PhysicsToolsUI::PhysicsToolStruct::ShowUI() {

		ImVec2 _size(ImGui::GetCurrentWindow()->DC.ItemWidth * 0.5f, 0.f);

		if (mFilePathQueue.empty()) {

			if (ImGui::Button(U8("选取文件夹"), _size)) {

				if (_LoadFolder(mFolderPath)) {

					FindAllFolder(mFolderPath, mFilePathQueue, mExtension.c_str());

				}

			}

			if (ImGui::Button(U8("选取文件"), _size)) {

				if (_LoadFile(mFolderPath, mFilePathQueue, mExtension.c_str())) {

					/*FindAllFolder(mFolderPath, mFilePathQueue, mExtension.c_str());*/

				}

			}

			if (ImGui::Button(U8("导入列表"), _size)) {

				Echo::FileLoadSaveWinInfo info;

				char filterStr[TEXT_BUFFER_MAX_PATH] = "All File\0 *.*\0";

				memcpy(info.filter, filterStr, TEXT_BUFFER_MAX_PATH * sizeof(char));

				info.initialDir = Root::instance()->getRootResourcePath();

				std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');

				std::string path;

				if (EditorFileToolsManager::instance()->CreateLoadWindow(info,path))

				{

					DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(path.c_str()));

					if (!pDataStream.isNull())

					{

						std::string chunkHeader;

						pDataStream->readString(chunkHeader);

						if (chunkHeader == _getHeaderChunkID(mExtension)) {

							uint32 size = 0;

							pDataStream->readData(&size, 1);

							for (int i = 0; i != size; ++i) {

								std::string name;

								pDataStream->readString(name);

								mFilePathQueue.push_back(Name(name));

							}

						}

						pDataStream->close();

					}

				}

			}

		}

		else {

			if (mQueueIsWork) {

				if (ImGui::Button(U8("停止"), _size)) {

					mQueueIsWork = false;

				}

			}

			else if (ImGui::Button(U8("开始"), _size)) {

				mQueueIsWork = true;

			}

			if (!mQueueIsWork) {

				if (mUserDataShow != nullptr) {

					mUserDataShow(this);

				}

				if (!mVersionVector.empty()) {

					ShowVersionList(U8("导出版本"), mVersion, mVersionVector);

				}

				ImGui::Checkbox(U8("是否覆盖旧文件"), &mIsCover);

				ImGui::DragInt(U8("单次处理量"), &mWorkload, 1.0f, 1, 20);

			}



			ImGui::Text(U8("文件列表："));

			ImGui::SameLine();

			ImGui::Text("%d", (int)mFilePathQueue.size());

			ImGui::SameLine();

			if (ImGui::Button(U8("清空"))) {

				mFilePathQueue.clear();

			}



			if (mQueueIsWork) {

				int n = mWorkload;

				while (n > 0 && !mFilePathQueue.empty()) {

					Name filePath = mFilePathQueue.back();

					if (!mTaskFunction(filePath, this)) {

						std::string filePathStr = filePath.toString();

						_CheckFilePath(filePathStr, "", "");

						mFailPathQueue.push_back(Name(filePathStr));

					}

					mFilePathQueue.pop_back();

					--n;

				}

				if (mFilePathQueue.empty()) {

					mQueueIsWork = false;

				}

			}

		}



		if (!mFailPathQueue.empty() && ImGui::TreeNode(U8("失败列表"))) {

			if (ImGui::Button(U8("清空列表"))) {

				mFailPathQueue.clear();

			}

			ImGui::SameLine();

			if (ImGui::Button(U8("导出列表"))) {

				Echo::FileLoadSaveWinInfo info;

				char filterStr[TEXT_BUFFER_MAX_PATH] = "All File\0 *.*\0";

				memcpy(info.filter, filterStr, TEXT_BUFFER_MAX_PATH * sizeof(char));

				info.initialDir = Root::instance()->getRootResourcePath();

				std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');

				std::string savefile;

				std::string defExt;

				if (EditorFileToolsManager::instance()->CreateSaveWindow(info, defExt, savefile))

				{

					DataStreamPtr pDataStream(Root::instance()->CreateDataStream(savefile.c_str()));

					if (!pDataStream.isNull())

					{

						pDataStream->writeString(_getHeaderChunkID(mExtension));

						uint32 size = (uint32)mFailPathQueue.size();

						pDataStream->writeData(&size, 1);

						for (auto& name : mFailPathQueue) {

							pDataStream->writeString(name.toString());

						}

						pDataStream->close();

					}

				}

			}

			ImGui::SameLine();

			if (ImGui::Button(U8("加入文件列表"))) {

				mFilePathQueue.swap(mFailPathQueue);

				mFailPathQueue.clear();

			}

			for (auto& name : mFailPathQueue) {

				ImGui::Text(name.c_str());

			}

			ImGui::TreePop();

		}

	}



	void PhysicsToolsUI::PhysicsToolStruct::ShowUI2() {

		ImVec2 _size(ImGui::GetCurrentWindow()->DC.ItemWidth * 0.5f, 0.f);

		if (mFilePathQueue.empty()) {

			if (ImGui::Button(U8("选取文件夹"), _size)) {

				if (_LoadFolder(mFolderPath)) {

					FindAllFolder(mFolderPath, mFilePathQueue, mExtension.c_str());

				}

			}

			if (ImGui::Button(U8("选取文件"), _size)) {

				if (_LoadFile(mFolderPath, mFilePathQueue, mExtension.c_str())) {

					/*FindAllFolder(mFolderPath, mFilePathQueue, mExtension.c_str());*/

				}

			}

		}

		else {

			if (mQueueIsWork) {

				if (ImGui::Button(U8("停止"), _size)) {

					mQueueIsWork = false;

				}

			}

			else if (ImGui::Button(U8("开始"), _size)) {

				mQueueIsWork = true;

			}

			if (!mQueueIsWork) {

				if (!mVersionVector.empty()) {

					ShowVersionList(U8("导出版本"), mVersion, mVersionVector);

				}

				//ImGui::Checkbox(U8("是否覆盖旧文件"), &mIsCover);

				//ImGui::DragInt(U8("单次处理量"), &mWorkload, 1.0f, 1, 20);

			}



			ImGui::Text(U8("文件数量："));

			ImGui::SameLine();

			ImGui::Text("%d", (int)mFilePathQueue.size());

			ImGui::SameLine();

			if (ImGui::Button(U8("清空"))) {

				mFilePathQueue.clear();

			}



			if (mQueueIsWork) {

				while (!mFilePathQueue.empty()) {

					Name filePath = mFilePathQueue.back();

					String result = mTaskFunction2(filePath, this);

					mResultQueue.push_back(result);

					mFilePathQueue.pop_back();

				}

				if (mFilePathQueue.empty()) {

					mQueueIsWork = false;

				}

			}

		}

		if (!mResultQueue.empty() && ImGui::TreeNode(U8("测试结果"))) {

			if (ImGui::Button(U8("清空结果"))) {

				mResultQueue.clear();

			}

			ImGui::SameLine();

			if (ImGui::Button(U8("导出结果"))) {

				Echo::FileLoadSaveWinInfo info;

				char filterStr[TEXT_BUFFER_MAX_PATH] = "All File\0 *.*\0";

				memcpy(info.filter, filterStr, TEXT_BUFFER_MAX_PATH * sizeof(char));

				info.initialDir = Root::instance()->getRootResourcePath();

				std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');

				std::string savefile;

				std::string defExt;

				if (EditorFileToolsManager::instance()->CreateSaveWindow(info, defExt, savefile))

				{

					DataStreamPtr pDataStream(Root::instance()->CreateDataStream(savefile.c_str()));

					if (!pDataStream.isNull())

					{

						for (auto& name : mResultQueue) {

							String str = name + "\n";

							pDataStream->writeData(str.c_str(), str.size());

						}

						pDataStream->close();

					}

				}

			}

			for (auto& resultPair : mResultQueue) {

				ImGui::Text(resultPair.c_str());

			}

			ImGui::TreePop();

		}

	}



	void PhysicsToolsUI::PhysicsToolStruct::FindAllFolder(std::string& folderPath, std::vector<Name>& filePathQueue, const char* extension) {

		FindAllFile(folderPath, filePathQueue, extension);

		_finddata_t file_info;

		std::string current_path = folderPath + "/*.*";

		intptr_t handle = _findfirst(current_path.c_str(), &file_info);

		if (-1 == handle) {

			return;

		}

		do {

			if (file_info.attrib == _A_SUBDIR) {

				std::string FileName = file_info.name;

				if (FileName == "." || FileName == "..") {

					continue;

				}

				std::string FilePathInput = folderPath + "\\" + FileName;

				FindAllFolder(FilePathInput, filePathQueue, extension);

				continue;

			}

		} while (!_findnext(handle, &file_info));

		_findclose(handle);

	}



	void PhysicsToolsUI::PhysicsToolStruct::FindAllFile(std::string& folderPath, std::vector<Name>& filePathQueue, const char* extension) {

		_finddata_t file_info;

		std::string current_path = folderPath + "/*." + extension;

		intptr_t handle = _findfirst(current_path.c_str(), &file_info);

		int Number = 0;

		if (-1 == handle) {

			return;

		}

		do {

			if (file_info.attrib == _A_SUBDIR) {

				continue;

			}

			std::string FileName = file_info.name;

			std::string FilePathInput = folderPath + "\\" + FileName;

			filePathQueue.push_back(Name(FilePathInput));

			++Number;

		} while (!_findnext(handle, &file_info));

		_findclose(handle);

	}







	//void InteractorManagerUI::InitGui(int classID)

	//{

	//	mUIType = U8("交互管理界面");

	//	mUIName = U8("交互管理界面##") + std::to_string(classID);

	//	ImGui::SetNextWindowPos(mPosition, mPositionType);

	//	ImGui::SetNextWindowSize(mSize, mSizeType);

	//

	//}

	//

	//void InteractorManagerUI::Keyboard() {}

	//

	//

	//InteractorManagerUI::InteractorManagerUI(int classID, ImGuiContext* ctx)

	//	: BaseUI(classID, ctx)

	//	, mPosition(0.f, 0)

	//	, mPositionType(ImGuiCond_FirstUseEver)

	//	, mSize(300.f, 300)

	//	, mSizeType(ImGuiCond_FirstUseEver)

	//{

	//	InitGui(classID);

	//	IsCreated = true;

	//}

	//InteractorManagerUI::~InteractorManagerUI() {

	//	IsCreated = false;

	//}

	//

	//

	//void InteractorManagerUI::ShowGui() {

	//	if (!ImGui::BeginChild(mUIName.c_str())) {

	//		ImGui::EndChild();

	//		return;

	//	}

	//

	//	if (EchoIR::EchoInteractorManager::instance()->GetIsRunning()) {

	//		ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::LockRoleButtonColor);

	//		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::LockRoleButtonHoveredColor);

	//		if (ImGui::Button(U8("正在运行"), ImVec2(ImGui::GetWindowWidth(), 0.f))) {

	//			EchoIR::EchoInteractorManager::instance()->SetIsRunning(false);

	//		}

	//		ImGui::PopStyleColor(2);

	//	}

	//	else if (ImGui::Button(U8("未运行"), ImVec2(ImGui::GetWindowWidth(), 0.f))) {

	//		EchoIR::EchoInteractorManager::instance()->SetIsRunning(true);

	//	}

	//

	//	EchoIR::Interactor* pInter = EchoIR::EchoInteractorManager::instance()->GetActiveInteractor();

	//

	//	if (pInter) {

	//		//if (ImGui::TreeNodeEx("ActiveInteractor", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

	//		//	

	//		//	int index = 0;

	//		//	for (EchoIR::IntObjComponents& intObj : pInter->intOjbComponents) {

	//		//		ImGui::PushID(index);

	//		//		if (intObj.interactorObject) {

	//		//			if (ImGui::TreeNode(intObj.interactorObject->getCombinedName().c_str())) {

	//		//				const std::vector<EchoIR::InteractorTarget*>& _allTargets = intObj.interactorObject->childTargets;

	//		//				for (int i = 0; i != _allTargets.size(); ++i) {

	//		//					ImGui::Text(_allTargets[i]->getComName().c_str());

	//		//				}

	//		//				ImGui::TreePop();

	//		//			}

	//		//		}

	//		//		//bool IsCurSelect = (pInter->selectedByUI == index);

	//		//		//if (IsCurSelect) {

	//		//		//	ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::LockRoleButtonColor);

	//		//		//	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::LockRoleButtonHoveredColor);

	//		//		//}

	//		//		//if (ImGui::Button(intObj.interactorObject->getComName().c_str(), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.f))) {

	//		//		//	pInter->selectedByUI = index;

	//		//		//}

	//		//		//if (IsCurSelect) {

	//		//		//	ImGui::PopStyleColor(2);

	//		//		//}

	//		//		ImGui::PopID();

	//		//		++index;

	//		//	}

	//		//	//if (ImGui::Button(U8("使用"), ImVec2(ImGui::GetWindowWidth() * 0.8f, 0.f))) {

	//		//	//	pInter->StartStopInteractions(true);

	//		//	//}

	//		//	ImGui::TreePop();

	//		//}

	//		/*if (ImGui::TreeNodeEx("Interactor Objects in Range (Closest Sorted)", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) {

	//		

	//			float curWidth = ImGui::GetWindowWidth() * 0.6f;

	//			int index = 0;

	//			for (EchoIR::IntObjComponents& intObj : pInter->intOjbComponents) {

	//				ImGui::PushID(index);

	//

	//				bool IsSelectedByUI = index == pInter->selectedByUI;

	//				if (IsSelectedByUI) {

	//					ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::LocatorSelectButtonColor);

	//					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::LocatorSelectButtonColor);

	//				}

	//

	//				if (ImGui::Button(intObj.interactorObject->getComName().c_str(), ImVec2(curWidth, 0.f))) {

	//					pInter->selectedByUI = index;

	//				}

	//

	//				if (IsSelectedByUI) {

	//					ImGui::PopStyleColor(2);

	//				}

	//				ImGui::PopID();

	//				++index;

	//			}

	//			if (!pInter->intOjbComponents.empty()) {

	//				ImGui::PushStyleColor(ImGuiCol_Button, EditorFinalIK::FBBIKSelectButtonColor);

	//				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorFinalIK::FBBIKSelectButtonColor);

	//				if (ImGui::Button(U8("使用"), ImVec2(curWidth, 0.f))) {

	//					pInter->StartStopInteractions(false);

	//				}

	//				ImGui::PopStyleColor(2);

	//			}

	//		

	//			ImGui::TreePop();

	//		}*/

	//

	//		float curWidth = ImGui::GetWindowWidth() * 0.6f;

	//		int index = 0;

	//		for (EchoIR::IntObjComponents& intObj : pInter->intOjbComponents) {

	//			ImGui::PushID(index);

	//			if (ImGui::Button(intObj.interactorObject->getComName().c_str(), ImVec2(curWidth, 0.f))) {

	//				pInter->selectedByUI = index;

	//				pInter->StartStopInteractions();

	//			}

	//			ImGui::PopID();

	//			++index;

	//		}

	//	}

	//

	//	ImGui::EndChild();

	//}



};



