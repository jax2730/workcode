// Author: anhaoxiang
#include "EchoComponentStableHeaders.h"
#include "EchoTimelineComponent.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoActor.h"
#include "EchoResourceManagerFactory.h"
#include "EchoResourceFolderManager.h"
#include "EchoCameraBaseComponent.h"
#include "EchoCameraComponentManager.h"
#include "EchoAnimationTimelineComponent.h"
#include <ranges>
//#include "EchoHermiteCurve.h"
//#include "EchoHermite.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(TimelineComponent)

	EDITOR_REFLECT_BEGIN(TimelineComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TimelineFilePath, "TimelineFilePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	Name todTimelineType_ = Name("TodTimelineComponent");

	TimelineComponent::TimelineComponent(EchoWeakPtr<Actor> pActor, ActorCOMMoveableType isMoveable)
	:ProtocolActorComponent(pActor, isMoveable)
	{
	}

	TimelineComponent::~TimelineComponent()
	{
		ProcessRotationSeparately(-1, true);
	}
   
	float TimelineComponent::floatInterpolation(size_t vIdx, float weight , int& preFrame, int& nextFrame)
	{
		if (vIdx >= mHermites.size())
			return 0.0f;
	
		if (mHermites[vIdx]->mHermiteType != EchoHermite::Float_Hermite)
			return 0.f;
		else if (mHermites[vIdx]->mKeyMap.size() == 1)
		{
			HermiteFloatKey* hfKey = static_cast<HermiteFloatKey*> (mHermites[vIdx]->mKeyMap.begin()->second);
			return hfKey->value;
		}
		EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return 0.f;
		return  pHermite->Interpolate(weight  , preFrame, nextFrame);
	}

	int TimelineComponent::intInterpolation(size_t  vIdx, float weight , int& preFrame, int& nextFrame)
	{
		if (vIdx >= mHermites.size())
			return -1;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::Fragment_Hermite)
		   return -1;

		EchoFragmentHermite* pHermite = static_cast<EchoFragmentHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return -1;
		return  pHermite->Interpolate(weight,  preFrame, nextFrame);
	}

	bool TimelineComponent::boolInterpolation(size_t  vIdx, float weight ,int& preFrame, int& nextFrame)
	{
		if (vIdx >= mHermites.size())
			return false;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::Bool_Hermite)
			return false;

		EchoBoolHermite* pHermite = static_cast<EchoBoolHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return false;
		return  pHermite->Interpolate(weight, preFrame, nextFrame);
	}

	float TimelineComponent::colorInterpolation(size_t vIdx, float weight, int& preFrame, int& nextFrame)
	{
		if (vIdx >= mHermites.size())
			return 1.f;
		auto hermite = mHermites[vIdx];
		if (hermite == nullptr)
			return 0.f;
		EchoFloatHermite* floatHermite = static_cast<EchoFloatHermite*>(hermite);
		auto& hermiteMap = floatHermite->mKeyMap;
		if (hermiteMap.empty())
			return 0.f;
		auto currIter = hermiteMap.lower_bound(static_cast<int>(weight));

		if (currIter == hermiteMap.end())
			return floatHermite->GetFloatKey(hermiteMap.rbegin()->second)->value;
		if (currIter->first == static_cast<int>(weight))
			return floatHermite->GetFloatKey(currIter->second)->value;
		if (currIter == hermiteMap.begin())
			return floatHermite->GetFloatKey(currIter->second)->value;
		auto nextIter = currIter;
		-- currIter;

		float currValue = floatHermite->GetFloatKey(currIter->second)->value;
		float nextValue = floatHermite->GetFloatKey(nextIter->second)->value;
		int dx = nextIter->first - currIter->first;
		float dy = nextValue - currValue;
		float onePercentValue = dy / dx;
		float diffValue = (static_cast<int>(weight) - currIter->first) * onePercentValue;

		preFrame = currIter->first;
		nextFrame = nextIter->first;
		return currValue + diffValue;
	}

	void TimelineComponent::stringInterpolation(size_t vIdx, float weight,std::string& value_, int& preFrame)
	{
		if (vIdx > mHermites.size())
		{
			value_ = "";
			return;
		}
		if (mHermites[vIdx]->mHermiteType != EchoHermite::String_Hermite)
		{
			value_ = "";
			return;
		}
		EchoStringHermite* pHermite = static_cast<EchoStringHermite*>(mHermites[vIdx]);
		if (pHermite == nullptr)
			return;
		pHermite->Interpolate(weight,value_,preFrame);
			
	}


	bool TimelineComponent::getLastKeyValue(size_t vIdx, float frame, float& resultValue, int& preFrame)
	{
		if (vIdx >= mHermites.size())
			return false;
		if (mHermites[vIdx]->mHermiteType != EchoHermite::Float_Hermite)
			return false;

		EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return false;

		std::map<int, Echo::HermiteKey*>::const_iterator iter = pHermite->mKeyMap.lower_bound(static_cast<int>(frame));

		if (iter != pHermite->mKeyMap.end() && iter->first == static_cast<int>(frame))
		{
			resultValue = static_cast<HermiteFloatKey*>(iter->second)->value;
			preFrame = iter->first;
			return true;
		}

		if (iter == pHermite->mKeyMap.begin())
			resultValue = 0.f;
		else
		{
			iter--;
			HermiteFloatKey* key_ = static_cast<HermiteFloatKey*>(iter->second);
			if (!key_)
				return false;
			resultValue = key_->value;
			preFrame = iter->first;
		}
		return true;
	}

	Echo::HermiteKey* TimelineComponent::GetKey(size_t vIdx, int keyVal)
	{
		if (vIdx >= mHermites.size())
			return nullptr;
		if (mHermites[vIdx]->mKeyMap.find(keyVal) == mHermites[vIdx]->mKeyMap.end())
			return nullptr;
		return mHermites[vIdx]->mKeyMap[keyVal];
	}

	int TimelineComponent::cameraStateInterpolation(const size_t& vIdx, float weight)
	{
		if (vIdx > mHermites.size())
			return -1;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::Bool_Hermite)
			return -1;

		EchoBoolHermite* pHermite = static_cast<EchoBoolHermite*>(mHermites[vIdx]);
		if (pHermite == nullptr)
			return -1;

		int weightFrame_ = static_cast<int>(weight + 1e-5f);

		if (pHermite->mKeyMap.empty())
			return -1;
		if (pHermite->mKeyMap.begin()->first > weightFrame_)
			return -1;

		auto iter = pHermite->mKeyMap.lower_bound(weightFrame_);

		if (iter == pHermite->mKeyMap.end())
		{
			--iter;
			bool value_ = pHermite->GetBoolKey(iter->second)->value;
			if (value_)
				return iter->first;
			return -1;
		}

		bool value_ = false;
		if (iter->first == weightFrame_)
		{
			value_ = pHermite->GetBoolKey(iter->second)->value;
		}
		else
		{
			iter--;
			value_ = pHermite->GetBoolKey(iter->second)->value;
		}

		if(value_)
	 	   return iter->first;
		return -1;
	}

	bool TimelineComponent::judgeIsExitKeyFrame(int idx, int frame)
	{
		if (idx < 0 || idx >= mHermites.size())
			return false;
		if (!mHermites[idx])
			return false;
		if (mHermites[idx]->mKeyMap.count(frame) == 0)
			return false;
		return true;
	}

	void TimelineComponent::deleteRowKey(int idx)
	{
		if (idx >= mHermites.size())
			return;
		if (mHermites[idx] == nullptr)
			return;
		std::vector<int> frameVec(mHermites[idx]->mKeyMap.size());
		
		int i = 0;

		for (const auto& iter : mHermites[idx]->mKeyMap)
			frameVec[i++] = iter.first;

		for (int i = 0; i < frameVec.size(); ++i)
			DeleteKey(idx, frameVec[i]);
	}

	void TimelineComponent::AddProperty(const EchoHermite::HermiteType& type, const Name& typeName , const FragmentType& type_)
	{
		EchoHermite* hermite = nullptr;
		if (typeName == Name::g_cEmptyName)
		{
			if (type == EchoHermite::Float_Hermite)
				hermite = new EchoFloatHermite();
			else if (type == EchoHermite::Bool_Hermite)
				hermite = new EchoBoolHermite();
			else if (type == EchoHermite::Fragment_Hermite)
				hermite = new EchoFragmentHermite(type_);
			else if (type == EchoHermite::String_Hermite)
				hermite = new EchoStringHermite();
			else
				hermite = new EchoFloatHermite();
		}
		else
		{
			if (EchoHermiteFactortManager::getSingleton().getHermiteFactory(typeName) == nullptr)
				assert(!"this hermitekey is not register");
			hermite = EchoHermiteFactortManager::getSingleton().getHermiteFactory(typeName)->createHermite();
		}
		
		mHermites.push_back(hermite);

#ifdef ECHO_EDITOR
		showCurveVec.push_back(1);
        HermiteNames.push_back("");
#endif
	}

#ifdef ECHO_EDITOR
	void TimelineComponent::RemoveProperty(const size_t & vIdx)
	{
		if (vIdx >= mHermites.size())
			return;
		HermiteNames.erase(HermiteNames.begin() + vIdx);

		auto hermite = mHermites[vIdx];
		auto& keyMap = hermite->mKeyMap;
		for (auto& item : keyMap)
		{
			if(item.second)
			   delete(item.second);
			item.second = nullptr;
		}
		delete(mHermites[vIdx]);
		mHermites[vIdx] = nullptr;
		mHermites.erase(mHermites.begin() + vIdx);
		showCurveVec.erase(showCurveVec.begin() + vIdx);
		for (auto& hermite : mHermites)
		{
			mMaxWeight = std::max(mMaxWeight, hermite->GetMaxLength());
		}
	}

	void TimelineComponent::RemoveGroupProperty(size_t  st, size_t  en)
	{
		if (st >= mHermites.size())
			return;
		en = std::min(en, mHermites.size());
		if (st < 0)
			st = 0;
		for(size_t i = st; i < en; i++)
		{
			auto& keyMap = mHermites[i]->mKeyMap;
			for (auto& item : keyMap)
			{
				if(item.second)
				   delete(item.second);
				item.second = nullptr;
			}
			SAFE_DELETE(mHermites[i]);
        }

		HermiteNames.erase(HermiteNames.begin() + st, HermiteNames.begin()+en);
		mHermites.erase(mHermites.begin() + st,mHermites.begin()+en);

		showCurveVec.erase(showCurveVec.begin() + st , showCurveVec.begin() + en);

		for (auto& hermite : mHermites)
		{
		    mMaxWeight = std::max(mMaxWeight, hermite->GetMaxLength());
		}

	}
	void TimelineComponent::addCameraBaseComData(const EchoWeakPtr<ActorComponent>& cameraCom_)
	{
		if (cameraCom_.expired())
			return;
		
		if (mCameraBaseComMap.empty())
		{
			mCameraMgrIdx = OrderCnt;
			++OrderCnt;
			importItem(EchoHermite::HermiteType::String_Hermite);
		}

		int tempCount = 1;
		if (mCameraBaseComMap.empty())
			++tempCount;
		if(mCameraBaseComMap.count(cameraCom_) == 0)
			mCameraBaseComMap.insert(std::make_pair(cameraCom_,OrderCnt - tempCount));

	}
	void TimelineComponent::deleteCameraBaseComData(const EchoWeakPtr<ActorComponent>& cameraCom_)
	{
		if (cameraCom_.expired())
			return;
		int currIdx = -1;
		if (mCameraBaseComMap.count(cameraCom_) != 0)
		{
			currIdx = mCameraBaseComMap[cameraCom_];
			mCameraBaseComMap.erase(cameraCom_);
		}
		if (mCameraBaseComMap.empty())
		{
			RemoveGroupProperty(mCameraMgrIdx, mCameraMgrIdx+1);
			mCameraMgrIdx = -1;
		}

		if (mCameraMgrIdx != -1 && currIdx != -1 && !mCameraFrameToIdx.empty())
		{
			std::map<int, int>::iterator iter = mCameraFrameToIdx.begin();

			std::set<int> deleteFrameSet;
 			while (iter != mCameraFrameToIdx.end())
			{
				if (iter->second == currIdx)
					deleteFrameSet.insert(iter->first);
				iter++;
			}
			for(int frame : deleteFrameSet)
			    deleteCameraMgrKey(frame);
		}
	}
	void TimelineComponent::addCameraMgrKey(int frame_, int vIdx_, const ActorComponentPtr& cameraCom_)
	{
		if (cameraCom_.expired())
			return;
		mCameraFrameToIdx[frame_] = vIdx_;
		if (mCameraMgrIdx != -1)
		{
			AddKey(mCameraMgrIdx,frame_,cameraCom_->getComNameToString());
			AddKey(vIdx_, frame_, true);
		}
	}
	void TimelineComponent::deleteCameraMgrKey(int frame_)
	{
		size_t count_ = mCameraFrameToIdx.count(frame_);
		int idx = -1;
		if (count_ != 0)
		{
			idx = mCameraFrameToIdx[frame_];
			mCameraFrameToIdx.erase(frame_);
		}
		if (mCameraMgrIdx != -1)
		{
			DeleteKey(mCameraMgrIdx, frame_);
			if (idx != -1)
				DeleteKey(idx, frame_);
		}

	}
	int TimelineComponent::getCameraComIdx(const ActorComponentPtr& cameraCom_)
	{
		int idx = -1;
		if (cameraCom_.expired())
			return idx;

		if (mCameraBaseComMap.count(cameraCom_) == 0)
			return idx;
		idx = mCameraBaseComMap[cameraCom_];

		return idx;
	}
	void TimelineComponent::getCameraComCombineNameByIdx(int idx, std::string& name_)
	{
		for (const auto& iter : mCameraBaseComMap)
		{
			if (!iter.first.expired() && iter.second == idx)
			{
				name_ = iter.first->getCombinedName();
				break;
			}
		}
	}
	void TimelineComponent::getCameraComNameByIdx(int idx, std::string& name_)
	{
		for (const auto& iter : mCameraBaseComMap)
		{
			if (!iter.first.expired() && iter.second == idx)
			{
				name_ = iter.first->getComName();
				break;
			}
		}
	}
	
#endif
	void TimelineComponent::AddKey(size_t  vIdx, int frame, float value, bool flag_)
	{
		if (vIdx >= mHermites.size())
			return;
		auto& item = mHermites[vIdx];
		if (item->mHermiteType != EchoHermite::Float_Hermite)
			return;
		EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(item);
		pHermite->CreateKey(frame, value);
		mMaxWeight = std::max(mMaxWeight, mHermites[vIdx]->GetMaxLength());
#ifdef ECHO_EDITOR
		addKeySetData(static_cast<int>(vIdx), frame);
#endif
		if(!bPlayScene && flag_)
		   setAttributeData(vIdx);
	}

	void TimelineComponent::AddKey(size_t  vIdx, int  frame, bool  value)
	{
		if (vIdx >= mHermites.size())
			return;
		auto& item = mHermites[vIdx];
		if (item->mHermiteType != EchoHermite::Bool_Hermite)
			return;
		auto pHermite = static_cast<EchoBoolHermite*>(item);
		pHermite->CreateKey(frame, value);
		mMaxWeight = std::max(mMaxWeight,mHermites[vIdx]->GetMaxLength());
		
#ifdef ECHO_EDITOR
		//相机关键帧特判
		addKeySetData(static_cast<int>(vIdx), frame);
		if (mCameraMgrIdx != -1 && !mIsMoveGroupKey)
		{
			mCameraFrameToIdx[frame] = static_cast<int>(vIdx);
			if (mCameraMgrIdx < mHermites.size())
			{
				std::string comName_ = "";
				getCameraComNameByIdx(static_cast<int>(vIdx), comName_);
				AddKey(mCameraMgrIdx, frame, comName_);
			}
		}
#endif

		if(!bPlayScene)
			setAttributeData(vIdx);
	}

	void TimelineComponent::AddKey(size_t  vIdx, int  frame, int  value)
	{
		if (vIdx >= mHermites.size())
			return;
		auto& item = mHermites[vIdx];
		if (item->mHermiteType != EchoHermite::Fragment_Hermite)
			return;
		auto pHermite = static_cast<EchoFragmentHermite*>(item);
		pHermite->CreateKey(frame, value);
		mMaxWeight = std::max(mMaxWeight, mHermites[vIdx]->GetMaxLength());
#ifdef ECHO_EDITOR
		addKeySetData(static_cast<int>(vIdx), frame);
#endif
	}

	void TimelineComponent::AddKey(size_t vIdx, int frame, const std::string& value)
	{
		if (vIdx >= mHermites.size())
			return;
		auto& item = mHermites[vIdx];
		if (item->mHermiteType != EchoHermite::String_Hermite)
			return;
		auto pHermite = static_cast<EchoStringHermite*>(item);
		pHermite->CreateKey(frame, value);
		mMaxWeight = std::max(mMaxWeight, mHermites[vIdx]->GetMaxLength());
#ifdef ECHO_EDITOR
		if (vIdx != mCameraMgrIdx)
			addKeySetData(static_cast<int>(vIdx), frame);
#endif
		if (!bPlayScene)
			setAttributeData(vIdx);
	}

	void TimelineComponent::AddKeyWithHandTangent(size_t vIdx, int frame, float value, float leftHandTangent, float rightHandTangent)
	{
		AddKey(vIdx,frame,value);
		if (vIdx >= mHermites.size())
			return;
		auto& item = mHermites[vIdx];
		if (item->mHermiteType != EchoHermite::Float_Hermite)
			return;
		EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(item);
		if (!pHermite || pHermite->mKeyMap.count(frame) == 0)
			return;
		pHermite->KeyToBroken(frame);
		pHermite->mKeyMap[frame]->leftHandTangent = leftHandTangent;
		pHermite->mKeyMap[frame]->rightHandTangent = rightHandTangent;

	}


	/*void TimelineComponent::AddKey(size_t vIdx, int frame, HermiteKey* key)
	{
		if (!key ||  vIdx >= mHermites.size())
			return;
		if (!mHermites[vIdx])
			return;
		mHermites[vIdx]->mKeyMap[frame] = key;
		mMaxWeight = std::max(mMaxWeight, mHermites[vIdx]->GetMaxLength());
		if (!bPlayScene)
			setAttributeData(vIdx);
#ifdef ECHO_EDITOR
		if (vIdx != mCameraMgrIdx)
			addKeySetData(static_cast<int>(vIdx), frame);
#endif
	}*/


	void TimelineComponent::DeleteKey(size_t vIdx, int weight)
	{
		if (vIdx >= mHermites.size())
			return;
		EchoHermite* item = mHermites[vIdx];
		if (item->mHermiteType == EchoHermite::Float_Hermite)
		{
			EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(item);
			pHermite->RemoveKey(weight);
		}
		else if (item->mHermiteType == EchoHermite::Fragment_Hermite)
		{
			EchoFragmentHermite* pHermite = static_cast<EchoFragmentHermite*>(item);
			pHermite->RemoveKey(weight);
		}
		else if (item->mHermiteType == EchoHermite::Bool_Hermite)
		{
			EchoBoolHermite* pHermite = static_cast<EchoBoolHermite*>(item);
			pHermite->RemoveKey(weight);

#ifdef ECHO_EDITOR
			//相机关键帧特判
			if (!mIsMoveGroupKey && mCameraFrameToIdx.count(weight) != 0 && mCameraFrameToIdx[weight] == vIdx)
			{
				mCameraFrameToIdx.erase(weight);
				if(mCameraMgrIdx != -1 && mCameraMgrIdx < mHermites.size())
				   DeleteKey(mCameraMgrIdx,weight);
			}
#endif
		}
		else if (item->mHermiteType == EchoHermite::String_Hermite)
		{
			EchoStringHermite* pHermite = static_cast<EchoStringHermite*>(item);
			pHermite->RemoveKey(weight);
		}
		mMaxWeight = mHermites[vIdx]->GetMaxLength();
#ifdef ECHO_EDITOR
		if (vIdx != mCameraMgrIdx)
		    deleteKeySetData(static_cast<int>(vIdx), weight);
#endif
		if (!bPlayScene)
			setAttributeData(vIdx);
	}

	void TimelineComponent::SetKeyValue(size_t vIdx, int keyVal, float value)
	{
		if (vIdx >= mHermites.size())
			return;
		if (mHermites[vIdx]->mKeyMap.find(keyVal) == mHermites[vIdx]->mKeyMap.end())
			return;
		HermiteFloatKey* hfKey = static_cast<HermiteFloatKey*>(mHermites[vIdx]->mKeyMap[keyVal]);
		hfKey->value = value;
		if (!bPlayScene)
		   setAttributeData(vIdx);
	}

	void TimelineComponent::SetKeyValue(size_t vIdx, int keyVal, bool value)
	{
		if (vIdx >= mHermites.size())
			return;
		if (mHermites[vIdx]->mKeyMap.find(keyVal) == mHermites[vIdx]->mKeyMap.end())
			return;
		HermiteBoolKey* hfKey = static_cast<HermiteBoolKey*>(mHermites[vIdx]->mKeyMap[keyVal]);
		hfKey->value = value;
	}

	void TimelineComponent::SetKeyValue(size_t vIdx, int keyVal, const HermiteFragmentKey::FragmentObject & value)
	{
		if (vIdx >= mHermites.size())
			return;
		if (mHermites[vIdx]->mKeyMap.find(keyVal) == mHermites[vIdx]->mKeyMap.end())
			return;
		HermiteFragmentKey* hfKey = static_cast<HermiteFragmentKey*>(mHermites[vIdx]->mKeyMap[keyVal]);
		hfKey->value = value;
	}

	void TimelineComponent::SetKeyValue(size_t vIdx, int keyVal, const std::string& value)
	{
		if (vIdx >= mHermites.size())
			return;
		if (mHermites[vIdx]->mKeyMap.find(keyVal) == mHermites[vIdx]->mKeyMap.end())
			return;
		auto hfKey = static_cast<HermiteStringKey*>(mHermites[vIdx]->mKeyMap[keyVal]);
		hfKey->value = value;
	}

	void TimelineComponent::MoveKey(size_t vIdx, int oldT, int newT)
	{
		if (oldT == newT)
			return;
		if (vIdx >= mHermites.size()  || !mHermites[vIdx]->mKeyMap.contains(oldT))
			return;
		HermiteKey* oldKey = mHermites[vIdx]->mKeyMap[oldT];
		if (!oldKey)
			return;
		HermiteKey* newKey = mHermites[vIdx]->CreateKey();
		oldKey->Copy(newKey);

#ifdef ECHO_EDITOR
		//相机关键帧特判
		bool isCameraKey = false;
		if (!mIsMoveGroupKey && mCameraFrameToIdx.count(oldT) != 0 && mCameraFrameToIdx[oldT] == vIdx)
		{
			mCameraFrameToIdx.erase(oldT);
			isCameraKey = true;
		}
#endif

		DeleteKey(vIdx, oldT);
		if (mHermites[vIdx]->mKeyMap.find(newT) != mHermites[vIdx]->mKeyMap.end())
		{
			DeleteKey(vIdx, newT);
		}

		mHermites[vIdx]->mKeyMap.emplace(newT, newKey);
		mMaxWeight = std::max(mMaxWeight, mHermites[vIdx]->GetMaxLength());

#ifdef ECHO_EDITOR
		if (isCameraKey)
		{
			if (mCameraMgrIdx != -1 && mCameraMgrIdx < mHermites.size())
				MoveKey(mCameraMgrIdx, oldT, newT);
			mCameraFrameToIdx[newT] = static_cast<int>(vIdx);
		}
		if (vIdx != mCameraMgrIdx)
			moveKeySetData(static_cast<int>(vIdx), oldT, newT);
#endif
		auto type_ = mHermites[vIdx]->mHermiteType;
		if ((type_ == EchoHermite::Float_Hermite || type_ == EchoHermite::String_Hermite)&& !bPlayScene)
			setAttributeData(vIdx);
		else if(type_ == EchoHermite::Fragment_Hermite)
			static_cast<HermiteFragmentKey*>(newKey)->value.startTime = newT;
		oldKey = nullptr;
		newKey = nullptr;
	}

	void TimelineComponent::MoveKey(size_t vIdx,int oldT,int  newT,float value)
	{
		if (vIdx >= mHermites.size() || !mHermites[vIdx]->mKeyMap.contains(oldT))
			return;
		HermiteKey* oldKey = mHermites[vIdx]->mKeyMap[oldT];
		if (!oldKey)
			return;
		HermiteKey* newKey = mHermites[vIdx]->CreateKey();
		oldKey->Copy(newKey);
		DeleteKey(vIdx, oldT);
		if (mHermites[vIdx]->mKeyMap.find(newT) != mHermites[vIdx]->mKeyMap.end())
		{
			DeleteKey(vIdx, newT);
		}

		EchoHermite::HermiteType type_ = mHermites[vIdx]->mHermiteType;
		if (type_ == EchoHermite::Float_Hermite)
			static_cast<HermiteFloatKey*>(newKey)->value = value;
		mHermites[vIdx]->mKeyMap.emplace(newT, newKey);



		mMaxWeight = std::max(mMaxWeight, mHermites[vIdx]->GetMaxLength());
#ifdef ECHO_EDITOR
		moveKeySetData(static_cast<int>(vIdx), oldT, newT);
#endif
		if ((type_ == EchoHermite::Float_Hermite || type_ == EchoHermite::String_Hermite) && !bPlayScene)
			setAttributeData(vIdx);
		oldKey = nullptr;
		newKey = nullptr;
	}

	void TimelineComponent::moveGroupKey(const std::vector<int>& idxVec, const std::vector<int>& frameVec, const int& diffFrame, const float &diffValue)
	{
		if (idxVec.size() <= 0)
			return;
		size_t sz = idxVec.size();

#ifdef ECHO_EDITOR
		//相机关键帧特判
		mIsMoveGroupKey = true;
		for (size_t i = 0; i < sz; ++i)
		{
			if (mCameraFrameToIdx.count(frameVec[i]) != 0 && mCameraFrameToIdx[frameVec[i]] == idxVec[i])
			{
				int newFrame = frameVec[i] + diffFrame;
				newFrame = std::max(0, newFrame);
				if (mCameraMgrIdx != -1 && mCameraMgrIdx < mHermites.size() && frameVec[i] != newFrame)
				{
					mCameraFrameToIdx.erase(frameVec[i]);
					MoveKey(mCameraMgrIdx, frameVec[i], newFrame);
					mCameraFrameToIdx[newFrame] = static_cast<int>(idxVec[i]);
				}

			}
		}
#endif


		std::vector<HermiteKey*> oldKeyVec(sz,nullptr);

		for (int i = 0; i < sz; i++)
		{   
			_getOldKey(&oldKeyVec[i], idxVec[i], frameVec[i]);
			DeleteKey(idxVec[i], frameVec[i]);
		}

		for (int i = 0; i < sz; i++)
			_addKey(oldKeyVec[i], idxVec[i], frameVec[i], diffFrame, diffValue);

		for (int i = 0; i < sz; i++)
		{
			if (oldKeyVec[i] != nullptr)
			{
                delete(oldKeyVec[i]); 
				oldKeyVec[i] = nullptr;
			}
			   
		}

#ifdef ECHO_EDITOR
		mIsMoveGroupKey = false;
		moveKeySetData(idxVec, frameVec, diffFrame);
#endif

		for (const int& idx : idxVec)
		{
			if (mHermites[idx]->mHermiteType == EchoHermite::Float_Hermite && !bPlayScene)
				setAttributeData((size_t)idx);
		}
	    
	}

	void TimelineComponent::scaringKey(const std::map<int, std::pair<int, std::set<int>>>& scaringSetMap)
	{
		
#ifdef ECHO_EDITOR
		//相机关键帧特判
		mIsMoveGroupKey = true;
		for (const auto& frameIter : scaringSetMap)
		{
			for (int idxIter : frameIter.second.second)
			{
				if (mCameraFrameToIdx.count(frameIter.first) != 0 && mCameraFrameToIdx[frameIter.first] == idxIter)
				{
					int newFrame = frameIter.first + frameIter.second.first;
					newFrame = std::max(0, newFrame);
					if (mCameraMgrIdx != -1 && mCameraMgrIdx < mHermites.size() && frameIter.first != newFrame)
					{
						mCameraFrameToIdx.erase(frameIter.first);
						MoveKey(mCameraMgrIdx, frameIter.first, newFrame);
						mCameraFrameToIdx[newFrame] = idxIter;
					}
				}
			}
		}

		std::set<int> idxSet_;

		std::map<int, std::map<int, HermiteKey*>> resuleKeyMap;
		for (const auto& frameIter : scaringSetMap)
		{
			float scale = (float)(frameIter.first + frameIter.second.first) / frameIter.first;
			for (int idxIter : frameIter.second.second)
			{
				idxSet_.insert(idxIter);

				HermiteKey* tempKey = nullptr;
				_getOldKey(&tempKey,idxIter,frameIter.first);
				resuleKeyMap[frameIter.first][idxIter] = tempKey;

				if (scale != 0.f)
				{
					tempKey->leftHandTangent /= scale;
					tempKey->rightHandTangent /= scale;
				}
				tempKey = nullptr;

				DeleteKey(idxIter, frameIter.first);
			}
		}

		for (const auto& frameIter : scaringSetMap)
		{
			for (int idxIter : frameIter.second.second)
			{
				_addKey(resuleKeyMap[frameIter.first][idxIter],idxIter,frameIter.first, frameIter.second.first);
			}
		}

		mIsMoveGroupKey = false;

		
		for (auto& frameIter : resuleKeyMap)
		{
			for (auto& keyIter : frameIter.second)
			{
				if (keyIter.second)
					delete keyIter.second;
				keyIter.second = nullptr;
			}
		}
		resuleKeyMap.clear();


		for (int i : idxSet_)
		{
			if (i < mHermites.size() && mHermites[i]->mHermiteType == EchoHermite::Float_Hermite && !bPlayScene)
				setAttributeData((size_t)i);
		}
#endif
	}

	void TimelineComponent::init(ActorComponentInfo * info)
	{
		setPriority(20000);
		ProtocolActorComponent::init(info);
#ifdef ECHO_EDITOR
		if (TimelineName == "")
			TimelineName = getComNameToString();
		CallProtocolFunc(Name("GetEditorAddCom"), (void*)&mComTypeMaps, 0);
		CallProtocolFunc(Name("EditorAddTimeline"), nullptr, (ProtocolHandle)this);
#endif
		if (info)
		{
			TimelineComponentInfo* timeInfo_ = static_cast<TimelineComponentInfo*>(info);
			m_TimelineFilePath = timeInfo_->m_TimelineFilePath;
		}
		initOldTimeData();

		if (m_pActor && m_pActor->getCutSceneCom() != nullptr)
		{
			m_bIsCollectResource = true;
		}
	}

	void TimelineComponent::unInit()
	{
		ProtocolActorComponent::unInit();
		for (int i = 0; i < AttributeComponents.size(); ++i)
			AttributeComponents[i] = nullptr;
		mComListenerVecs.clear();
		for (auto& hermite : mHermites)
		{
			for (auto&hKey : hermite->mKeyMap)
			{
				SAFE_DELETE(hKey.second);
			}
			SAFE_DELETE(hermite);
		}
		mHermites.clear();

		for (const comMap::value_type& comIter : mComMaps)
		{
			if(!comIter.second.expired())
			   comIter.second->setTimelineCom(nullptr);
		}

#ifdef ECHO_EDITOR
		CallProtocolFunc(Name("EditorDeleteTimeline"), nullptr, (ProtocolHandle)this);
#endif
		TimeLineInfoLoadingTood::instance()->eraseMissingTimeline(this);
	}

	void TimelineComponent::connectCom()
	{
		if (m_TimelineFilePath != "")
		{
			bool loadState = false;
			if (m_pActor)
				loadState = m_pActor->isSyncLoadActor();

			GetTimeLineFile(m_TimelineFilePath.data(), loadState);
		}
		else
			m_isCreateFinish = true;
	}

	void TimelineComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}

	void TimelineComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_PrePhysicsUpdate].insert(EID_EveryFrame);
		m_eventSet[TT_PreUpdateScene].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void TimelineComponent::prePhysicsUpdate(EventID eventId, ProtocolHandle handle)
	{
		if (getType() == todTimelineType_)
			return;

		if (m_bIsCollectResource  && !m_LoadTimelineObjectSet.empty())
		{
			m_LoadObjectTickSecond += Echo::Root::instance()->getTimeSinceLastFrameClient();
			if (m_LoadObjectTickSecond >= m_LoadTimeoutData)
			{
				m_LoadTimelineObjectSet.clear();
				_loadFinish();
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "Timeline :load resource time out: " + m_TimelineFilePath);
			}
		}

		switch (eventId)
		{
		case EID_EveryFrame:
		{
			if (bPlayScene)
			{
				if (currentPlayWay == ONCE_PLAY)
					runOnceVideo();
				else if (currentPlayWay == LOOP_PLAY)
					runLoopVideo();
				else if (currentPlayWay == PING_PANG_PLAY)
					runPingPangVideo();

				setComponentAttribute();

				if (currentPlayWay == ONCE_PLAY)
				{
					if (mIsReversePlay)
					{
						if (currTime <= startSceneTime)
						{
							bPlayScene = false;
							playEnd();
						}
					}
					else
					{
						if (currTime >= endSceneTime)
						{
							bPlayScene = false;
							playEnd();
						}
							
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}

	void TimelineComponent::visibleStateChanged(bool visible)
	{
		
		if (!visible)
			playEnd();

		if ((visible && !mIsReversePlay) || (!visible && mIsReversePlay))
			mPreTime = startSceneTime;
		else
			mPreTime = endSceneTime;
	}

	void TimelineComponent::copyTo(const Echo::ActorComponentPtr& p_actor_component)
	{
	}

	std::string  IntToString(int num)
	{
		std::string result = "";
		while (num >= 10)
		{
			int temp = num % 10;
			result += ('0' + temp);
			num /= 10;
		}
		result += '0' + num;
		reverse(result.begin(),result.end());
		return result;
	}


	

	void TimelineComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			{
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "Timeline :File not exit: " + resName.toString());
			}
            return;
		}
	    m_PTimelineInfo =  static_cast<TimeLineInfoPtr>(resourcePtr);
		resetTimelineData(m_PTimelineInfo,true);
		m_TimelineFilePath = resName.toString();
		setComponentAttribute();

		_loadFinish();

#ifdef ECHO_EDITOR
		if (TimelineName.empty())
		{
			TimelineName = getComNameToString();
		}
#endif
	}


	bool TimelineComponent::isCreateFinish()
	{
		return m_isCreateFinish;
	}

	void TimelineComponent::initOldTimeData()
	{
		if (m_pActComInfo == nullptr)
			return;
		TimelineComponentInfo* TimeLineInfo = static_cast<TimelineComponentInfo*>(m_pActComInfo);
		if (TimeLineInfo->isLoadHermite)
			copyHermiteDatas(TimeLineInfo->mHermites, mHermites);
		
		if (TimeLineInfo->isLoadDeltaTime)
			mDeltaTime = TimeLineInfo->mDeltaTime;

		if (TimeLineInfo->isLoadStartTime)
			startSceneTime = TimeLineInfo->startSceneTime;

		if (!TimeLineInfo->isLoadEndTime)
			endSceneTime = TimeLineInfo->endSceneTime;

		if (!TimeLineInfo->isLoadPlayWay)
			currentPlayWay = TimeLineInfo->currentPlayWay;

	}

	float TimelineComponent::floatInterpolation(size_t vIdx, float weight)
	{
		if (vIdx >= mHermites.size())
			return 0.0f;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::Float_Hermite)
			return 0.f;
		else if (mHermites[vIdx]->mKeyMap.size() == 1)
		{
			HermiteFloatKey* hfKey = static_cast<HermiteFloatKey*> (mHermites[vIdx]->mKeyMap.begin()->second);
			return hfKey->value;
		}
		EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return 0.f;
		return  pHermite->Interpolate(weight);
	}

	int TimelineComponent::intInterpolation(size_t vIdx, float weight)
	{
		if (vIdx >= mHermites.size())
			return -1;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::Fragment_Hermite)
			return -1;

		EchoFragmentHermite* pHermite = static_cast<EchoFragmentHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return -1;
		return  pHermite->Interpolate(weight);
	}

	bool TimelineComponent::boolInterpolation(size_t vIdx, float weight)
	{
		if (vIdx >= mHermites.size())
			return false;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::Bool_Hermite)
			return false;

		EchoBoolHermite* pHermite = static_cast<EchoBoolHermite*>(mHermites[vIdx]);
		if (!pHermite)
			return false;
		return  pHermite->Interpolate(weight);
	}

	float TimelineComponent::colorInterpolation(size_t vIdx, float weight)
	{
		if (vIdx >= mHermites.size())
			return 1.f;
		auto hermite = mHermites[vIdx];
		if (hermite == nullptr)
			return 0.f;
		EchoFloatHermite* floatHermite = static_cast<EchoFloatHermite*>(hermite);
		auto& hermiteMap = floatHermite->mKeyMap;
		if (hermiteMap.empty())
			return 0.f;
		auto currIter = hermiteMap.lower_bound(static_cast<int>(weight));

		if (currIter == hermiteMap.end())
			return floatHermite->GetFloatKey(hermiteMap.rbegin()->second)->value;
		if (currIter->first == static_cast<int>(weight))
			return floatHermite->GetFloatKey(currIter->second)->value;
		if (currIter == hermiteMap.begin())
			return floatHermite->GetFloatKey(currIter->second)->value;
		auto nextIter = currIter;
		--currIter;

		float currValue = floatHermite->GetFloatKey(currIter->second)->value;
		float nextValue = floatHermite->GetFloatKey(nextIter->second)->value;
		int dx = nextIter->first - currIter->first;
		float dy = nextValue - currValue;
		float onePercentValue = dy / dx;
		float diffValue = (static_cast<int>(weight) - currIter->first) * onePercentValue;
		return currValue + diffValue;
	}

	void TimelineComponent::stringInterpolation(size_t vIdx, float weight, std::string& value_)
	{
		if (vIdx > mHermites.size())
		{
			value_ = "";
			return;
		}
		if (mHermites[vIdx]->mHermiteType != EchoHermite::String_Hermite)
		{
			value_ = "";
			return;
		}
		EchoStringHermite* pHermite = static_cast<EchoStringHermite*>(mHermites[vIdx]);
		if (pHermite == nullptr)
			return;
		pHermite->Interpolate(weight, value_);
	}

	int TimelineComponent::getMaxLength()
	{
		return mMaxWeight;
	}

	// 传入一个参数来获取当前时间的第几行时间轴的值
	float TimelineComponent::getActorTimeValue(size_t  vIdx)
	{	
		int frame = static_cast<int>(currTime * mDeltaTime+0.5);
		int idx = -1;
		return floatInterpolation(vIdx, static_cast<float>(frame),idx,idx);
		
	}
	//设置时间
	void TimelineComponent::timelineUpdate(float time)
	{
		currTime = time;
	}

	void TimelineComponent::remakeTime()
	{
		currTime = startSceneTime;
		mPreTime = currTime;
		PingPangOrder = false;
	}

	void TimelineComponent::runOnceVideo()
	{	
		float depardTime = Echo::Root::instance()->getTimeSinceLastFrameClient() * mTimelinePlaySpeed;

		if (!mIsReversePlay)
		{
			if (currTime < endSceneTime)
			{
				currTime += depardTime;
				if (currTime > endSceneTime)
				{
					currTime = endSceneTime;
				}
			}
		}
		else
		{
			if (currTime > startSceneTime)
			{
				currTime -= depardTime;
				if (currTime < startSceneTime)
				{
                    currTime = startSceneTime;
				}                 
			}
		}
	}

	void TimelineComponent::setPlayWay(PlayVideoType playWay_)
	{
	    currentPlayWay = playWay_;
	}

	PlayVideoType TimelineComponent::getPlayWay()
	{
		return static_cast<PlayVideoType>(currentPlayWay);
	}

	void TimelineComponent::runLoopVideo()
	{

		if (!mIsReversePlay)
		{
			if (currTime == endSceneTime)
			{
				currTime = startSceneTime;
				mPreTime = currTime;
			}
		}
		else
		{
			if (currTime == startSceneTime)
			{
				currTime = endSceneTime;
				mPreTime = currTime;
			}
		}

		float depardTime = Echo::Root::instance()->getTimeSinceLastFrameClient() * mTimelinePlaySpeed;

		if (!mIsReversePlay)
		{
			currTime += depardTime;
			if (currTime > endSceneTime)
			{
                 currTime = endSceneTime;
				 mPreTime = currTime;
			}
				
		}
		else
		{
			currTime -= depardTime;
			if (currTime < startSceneTime)
			{
               currTime = startSceneTime;
			   mPreTime = currTime;
			}
				
		}
	}

	void TimelineComponent::runPingPangVideo()
	{
		if ((!PingPangOrder && !mIsReversePlay)  || (PingPangOrder && mIsReversePlay))
		{
			currTime += Echo::Root::instance()->getTimeSinceLastFrameClient() * mTimelinePlaySpeed;
			if (currTime >= endSceneTime)
			{
				mPreTime = endSceneTime;
				float diffTime = currTime - endSceneTime;
                currTime = endSceneTime - diffTime;
				PingPangOrder = !PingPangOrder;
			} 
				
		}
		else
		{
			currTime -= Echo::Root::instance()->getTimeSinceLastFrameClient() * mTimelinePlaySpeed;
			if (currTime <= startSceneTime)
			{
				mPreTime = startSceneTime;
				float diffTime = startSceneTime - currTime;
				currTime = startSceneTime + diffTime;
				PingPangOrder = !PingPangOrder;
			}
		}
	}

	void TimelineComponent::setComponentAttribute()
	{
		TimelineTickData pTimelineTickData;

		for (const actorMapping::value_type& actorIter : mActorToComponents)
		{
			ActorPtr currActor = nullptr;
			
			if (mActorMaps.find(actorIter.first) != mActorMaps.end())
			{
				if (!mActorMaps[actorIter.first].expired())
					currActor = mActorMaps[actorIter.first];
			}
			if (currActor == nullptr)
				continue;
			for (const std::string& comIter : actorIter.second)
			{
				EchoWeakPtr<ActorComponent>  currCom = nullptr;
				if (mComMaps.find(comIter) != mComMaps.end())
				{
					if (!mComMaps[comIter].expired())
						currCom = mComMaps[comIter];
				}
				if (currCom == nullptr)
					continue;
				if (mComponentToAttributes.find(comIter) == mComponentToAttributes.end())
					continue;

				currCom->setTLPlaySpeed(m_fTimelineSpeed * getTimelinePlaySpeed());

				for (const attributeData& attrIter : mComponentToAttributes[comIter])
				{
					int st = attrIter.st;
					int en = attrIter.en;

#ifdef ECHO_EDITOR
					if (mIsRootMotionMode)
					{
						bool motionFlag = judgeIsRootMotionNode(attrIter);
						if (motionFlag)
							continue;
					}
#endif
					if (st >= mHermites.size())
						continue;
					const int sz = en - st;

					pTimelineTickData.floatData.resize(sz);
					pTimelineTickData.stringData.resize(sz);
					pTimelineTickData.preKey.resize(sz, nullptr);
					pTimelineTickData.nextKey.resize(sz, nullptr);


					bool isString = false;
					bool isSetAttr_ = false;

					if (attrIter.isQuaternion && attrIter.attributeName == rotationName_)
					{	//四元数插值
						float realFrame_ = getRealTime();
						Vector3 result_;
						QuaternionInterPolate(st, realFrame_, result_);
						pTimelineTickData.floatData[0] = result_.x;
						pTimelineTickData.floatData[1] = result_.y;
						pTimelineTickData.floatData[2] = result_.z;
						currCom->setTimelineAttribute(pTimelineTickData.floatData, attrIter.attributeName);

						//测试输出传入的插值中间数据

						//stringstream ss;
						//ss << "__ID__ : " << st
						//	<< "\t\t__frame__ : \t\t" << realFrame_
						//	<< "\t\t\t__Vector_x_y_z__ \t: " << resultEgl_.x << "\t\t" << resultEgl_.y << "\t\t" << resultEgl_.z << "\t\t" /*<< result.w << "\t\t"*/;
						//LogManager::instance()->logMessage(LML_NORMAL, ss.str());

						//ss.str("");
						//ss << "__ID__ : " << st
						//	<< "\t\t__frame__ : \t\t" << realFrame_
						//	<< "\t\t\t__Quaternion_x_y_z_w__ \t: " << result_.x << "\t" << result_.y << "\t" << result_.z << "\t" << result_.w << "\t";
						//LogManager::instance()->logMessage(LML_NORMAL, ss.str());

						continue;
					}
					else
					{
						isSetAttr_ = getHermiteData(attrIter, isString, pTimelineTickData);
					}

#ifdef ECHO_EDITOR
					if (recoredRootMotionData(attrIter, pTimelineTickData.floatData))
					{
						mIsUpdateRootMotionData = true;
						continue;
					}
#endif

					if (isSetAttr_)
					{
						if (attrIter.isAnimation)
						{
							for (int tickI = 0; tickI < pTimelineTickData.stringData.size(); ++tickI)
								if (!pTimelineTickData.stringData[tickI].empty())
									currCom->setTimelineStringAttribute(pTimelineTickData.stringData[tickI], pTimelineTickData.floatData[tickI], attrIter.attributeName); // ToAnimation
						}
						else if (isString)
							currCom->setTimelineStringAttribute(pTimelineTickData.stringData[0], attrIter.attributeName);
						else
							currCom->setTimelineAttribute(pTimelineTickData.floatData, attrIter.attributeName);


						currCom->setTimelineAttribute(pTimelineTickData, attrIter.attributeName);
					}
				}

#ifdef ECHO_EDITOR
				if (mIsUpdateRootMotionData)
				{
					increaseRootMotionNodeSkew(currCom.get());
					mIsUpdateRootMotionData = false;
				} 
#endif
				if (currCom->IsMoveable())
					currCom->UpdateComRootNode(true, false);

				currCom = nullptr;
			}
		}
		mPreTime = currTime;
#ifdef ECHO_EDITOR
		if (!recordComponentStates.empty())
			recordComponentStates.clear();
#endif
	}

	uint8 TimelineComponent::judgeCameraComIsActive(int vIdx, int frame_)
	{
		if (frame_ == -1)
			return 0;
		//帧大取帧大的，同帧取所在行数大的
		if (frame_ > mCameraActiveFrame)
			return 1;
		if (frame_ < mCameraActiveFrame)
			return 0;
		if (vIdx >= mCameraActiveIdx)
			return 1;
		return 0;
		
	}

	void TimelineComponent::setAttributeData(const size_t& vIdx)
	{
		if (bPlayScene)
			return;

		ProcessRotationSeparately(static_cast<int>(vIdx));

		TimelineTickData pTimelineTickData;

		for (const auto& comIter : mComponentToAttributes)
		{
			const EchoWeakPtr<ActorComponent>& com = mComMaps[comIter.first];
			if (com == nullptr)
				continue;
			for (const auto& attrIter : comIter.second)
			{
				const int sz = attrIter.en - attrIter.st;
				pTimelineTickData.floatData.resize(sz);
				pTimelineTickData.stringData.resize(sz);
				pTimelineTickData.preKey.resize(sz);
				pTimelineTickData.nextKey.resize(sz);
				if (attrIter.st <= vIdx && attrIter.en > vIdx)
				{
					bool isString = false;
					std::string stringValue_;
					bool isSetAttr_ = getHermiteData( attrIter, isString, pTimelineTickData);

#ifdef ECHO_EDITOR
					if (recoredRootMotionData(attrIter, pTimelineTickData.floatData))
					{
						mIsUpdateRootMotionData = true;
						continue;
					}
#endif
					//规整
					if (isSetAttr_)
					{
						if (attrIter.isAnimation)
						{
							for (int tickI = 0; tickI < pTimelineTickData.stringData.size(); ++tickI)
								if (!pTimelineTickData.stringData[tickI].empty())
									com->setTimelineStringAttribute(pTimelineTickData.stringData[tickI], pTimelineTickData.floatData[tickI], attrIter.attributeName); // ToAnimation
						}
						else if (isString)
							com->setTimelineStringAttribute(pTimelineTickData.stringData[0], attrIter.attributeName);
						else
							com->setTimelineAttribute(pTimelineTickData.floatData, attrIter.attributeName);

						com->setTimelineAttribute(pTimelineTickData, attrIter.attributeName);
					}
					return;
				}
#ifdef ECHO_EDITOR
				else
				{
					if (comIter.first == mRootMotionData.comName)
					{
						bool isString = false;
					
						std::vector<int> preFrame(pTimelineTickData.floatData.size()),nextFrame(pTimelineTickData.floatData.size());
						bool isSetAttr_ = getHermiteData(attrIter, isString, pTimelineTickData);
						if(isSetAttr_)
						   recoredRootMotionData(attrIter, pTimelineTickData.floatData);
					}
				}
#endif

			}
#ifdef ECHO_EDITOR
			if (mIsUpdateRootMotionData)
			{
				increaseRootMotionNodeSkew(com.get());
				mIsUpdateRootMotionData = false;
				return;
			}
#endif
		}
	}


	HermiteStringKey* TimelineComponent::TimeConsuming(size_t vIdx, float frame, float& time)
	{
		if (vIdx >= mHermites.size())
			return nullptr;

		if (mHermites[vIdx]->mHermiteType != EchoHermite::String_Hermite)
			return nullptr;

		EchoStringHermite* pHermite = static_cast<EchoStringHermite*>(mHermites[vIdx]);
		if (!pHermite || pHermite->mKeyMap.empty())
			return nullptr;

		// 若当前并未运行到任意滑块的起始点之后
		if (pHermite->mKeyMap.begin()->first / mDeltaTime > currTime)
			return nullptr;

		// 查询当前帧属于哪一滑块之后
		auto iter = pHermite->mKeyMap.lower_bound(static_cast<int>(frame));

		if (iter == pHermite->mKeyMap.end() || iter->first != static_cast<int>(frame))
			iter--;

		HermiteStringKey* pKey = pHermite->GetStringKey(iter->second);

		if (!pKey)
		{
			assert(!"[TimelineComponent] HermiteStringKey Value Error!");
			return nullptr;
		}

		float firstTime = (float)iter->first / mDeltaTime;
		float offsetTime = currTime - firstTime;

		// 已经运行到当前滑块结束之后与下一滑块开始之前的时间段
		if (offsetTime > pKey->mLength || offsetTime < 0.f)
			return nullptr;
		
		time = pKey->mStartTime + offsetTime;
		return pKey;
	}

	bool TimelineComponent::getHermiteData(const std::vector<Echo::attributeData>::const_iterator::value_type& iter, bool& isString, TimelineTickData& pTimelineTickData)
	{
		EchoHermite::HermiteType type_ = mHermites[iter.st]->mHermiteType;
		std::vector<int>  preFrame(pTimelineTickData.floatData.size(),-1);
		std::vector<int>  nextFrame(pTimelineTickData.floatData.size(),-1);

		bool result = false;
		float currFrame = getRealTime();
		for (int i = iter.st; i < iter.en; ++i)
		{
			if (iter.isAnimation)
			{
				if (type_ == EchoHermite::String_Hermite)
				{
					HermiteStringKey* pKey = TimeConsuming(i, currFrame, pTimelineTickData.floatData[i - iter.st]);
					if (pKey)
					{
						if (iter.isTruncation)
							pTimelineTickData.stringData[i - iter.st] = pKey->value;
						else
							pTimelineTickData.stringData[i - iter.st] = pKey->path;
						pTimelineTickData.preKey[i - iter.st] = pKey;
						pTimelineTickData.nextKey[i - iter.st] = pKey;
						result |= true;
					}
					else
						result |= false;
				}

				if (i + 1 == iter.en)
					return result;
			}
			else if (iter.isNoInterpolate)
			{
				switch (type_)
				{
					case EchoHermite::Bool_Hermite:
					{
						std::pair<float, bool> keyPair_ = std::make_pair(0.f, false);
						NoInterpolation<EchoBoolHermite, HermiteBoolKey, float>(i, currFrame, keyPair_, preFrame[i - iter.st]);
						if (keyPair_.second)
						{
							result |= true;
							pTimelineTickData.floatData[i - iter.st] = keyPair_.first == 1.f ? 1.f : -1.f;
						}
						else
						{
							pTimelineTickData.floatData[i - iter.st] = boolInterpolation(i, currFrame, preFrame[i - iter.st], nextFrame[i - iter.st]) ? 1.f : -1.f;
						}
						break;
					}
					case EchoHermite::Float_Hermite:
					{
						std::pair<float, bool> keyPair_ = std::make_pair(0.f, false);
						NoInterpolation<EchoFloatHermite, HermiteFloatKey, float>(i, currFrame, keyPair_, preFrame[i - iter.st]);
						if (keyPair_.second)
						{
							pTimelineTickData.floatData[i - iter.st] = keyPair_.first;
							result |= true;
						}
						else
						{
							getLastKeyValue(i, currFrame, pTimelineTickData.floatData[i - iter.st],preFrame[i - iter.st]);
						}
						break;
					}
					case EchoHermite::String_Hermite:
					{
						std::pair<std::string, bool> keyPair_ = std::make_pair(std::move(pTimelineTickData.stringData[i - iter.st]), false);
						NoInterpolation<EchoStringHermite, HermiteStringKey, std::string>(i, currFrame, keyPair_, preFrame[i - iter.st]);
						isString = true;
						if (keyPair_.second)
						{
							pTimelineTickData.stringData[i - iter.st] = keyPair_.first;
							result |= true;
						}
						else
						{
							stringInterpolation(i, currFrame, pTimelineTickData.stringData[i - iter.st], preFrame[i - iter.st]);
						}
						break;
					}
					default:
					{
						return  false;
						break;
					}
				}
				nextFrame[i - iter.st] = preFrame[i - iter.st];

				if (i + 1 == iter.en)
					return result;
			}
			else
			{
				switch (type_)
				{
					case EchoHermite::Bool_Hermite:
					{
						if (iter.isCamera)
						{
							int frame = cameraStateInterpolation(iter.st, currFrame);
							if (static_cast<int>(currFrame) < mCameraActiveFrame)
								pTimelineTickData.floatData[i - iter.st] = 1;
							else
								pTimelineTickData.floatData[i - iter.st] = judgeCameraComIsActive(iter.st, frame);
							if (pTimelineTickData.floatData[i - iter.st] == 1)
							{
								mCameraActiveFrame = frame;
								mCameraActiveIdx = iter.st;
							}
							else if (mCameraActiveFrame > frame && mCameraActiveIdx == iter.st)
							{
								mCameraActiveIdx = -1;
								setComponentAttribute();
								return false;
							}
						}
						else
						{
							bool flag = boolInterpolation(i, currFrame, preFrame[i - iter.st], nextFrame[i - iter.st]);
							if (flag)
								pTimelineTickData.floatData[i - iter.st] = 1;
							else
								pTimelineTickData.floatData[i - iter.st] = -1;
						}
						break;
					}
					case EchoHermite::Float_Hermite:
					{
						if (iter.isColor == 1)
							pTimelineTickData.floatData[i - iter.st] = colorInterpolation(i, currFrame, preFrame[i - iter.st], nextFrame[i - iter.st]);
						else
							pTimelineTickData.floatData[i - iter.st] = floatInterpolation(i, currFrame, preFrame[i - iter.st], nextFrame[i - iter.st]);
						break;
					}
					case EchoHermite::String_Hermite:
					{
						isString = true;
						stringInterpolation(i, currFrame, pTimelineTickData.stringData[i - iter.st], preFrame[i - iter.st]);
						break;
					}
					case EchoHermite::Fragment_Hermite:
					{
						int temp_ = intInterpolation(i, currFrame,preFrame[i - iter.st], nextFrame[i - iter.st]);
						pTimelineTickData.floatData[i - iter.st] = static_cast<float>(temp_);
						break;
					}
					default:
					{
						return false;
						break;
					}
				}
			}
		}

		for (int i = iter.st; i < iter.en; ++i)
		{
			if (preFrame[i - iter.st] != -1  && mHermites[iter.st]->mKeyMap.contains(preFrame[i - iter.st]))
				pTimelineTickData.preKey[i - iter.st] = mHermites[iter.st]->mKeyMap[preFrame[i - iter.st]];
			if (nextFrame[i - iter.st] != -1 && mHermites[iter.st]->mKeyMap.contains(nextFrame[i - iter.st]))
				pTimelineTickData.nextKey[i - iter.st] = mHermites[iter.st]->mKeyMap[nextFrame[i - iter.st]];
		}
		return true;
	}

	void TimelineComponent::setIsPlay(bool isplay_)
	{
		if (isplay_ != bPlayScene && !isplay_)
		{
			dealwithCameraComEndPlay();
		}

		bPlayScene = isplay_;
		if (isplay_)
		{
			if (currTime <= startSceneTime || currTime >= endSceneTime)
			{
				if (!mIsReversePlay)
					currTime = startSceneTime;
				else
					currTime = endSceneTime;
				mPreTime = currTime;
			}
		}
	}

	void TimelineComponent::playEnd()
	{
		dealwithCameraComEndPlay();

		for (const auto& iter : mComListenerVecs)
			iter->TimelineEndPlay(this);

		for (const comMap::value_type& iter : mComMaps)
		{
			if (!iter.second.expired())
				iter.second->timelinePlayEnd();
		}
	}

	bool TimelineComponent::getIsPlayScene()
	{
		return bPlayScene;
	}

	float TimelineComponent::getTimelinePlayPercent()
	{
		float totalTime = endSceneTime - startSceneTime;
		if (totalTime <= 0.f)
			return 0.f;
		float deltaTime = currTime - startSceneTime;
		if (deltaTime <= 0.f)
			return 0.f;
		return deltaTime / totalTime;
	}

	void TimelineComponent::setTimelineFilePath(const std::string& filePath_)
	{
		m_TimelineFilePath = filePath_;
	}
    
	bool GetQuaFrameRange(float t, size_t& leftIndex, size_t& rightIndex, const std::map<int, Quaternion>& quaMap)
	{
		if (leftIndex >= rightIndex)
			return false;
        
		if (leftIndex == rightIndex - 1)
			return true;

		size_t middle = leftIndex + (rightIndex - leftIndex) / 2;

		std::map<int, Quaternion>::const_iterator middleIter = std::next(quaMap.begin(), middle);

		if (t > middleIter->first)
			leftIndex = middle;
		else
			rightIndex = middle;

		return GetQuaFrameRange(t,leftIndex,rightIndex,quaMap);
	}

	float TimelineComponent::QuaternionInterPolate(int vIdx, float frame_, Vector3& result)
	{
		// 找到传入index对应到Attribute数据里的st值
		auto iter = std::find_if(mHermiteQuaternionData.begin(), mHermiteQuaternionData.end(), [vIdx](auto& a) { return a.first <= vIdx && a.first + 2 >= vIdx; });

		if (iter == mHermiteQuaternionData.end() || iter->second.empty())
			return 0.f;
		
		int id = vIdx;	// 记录当前传入的index
		
		vIdx = iter->first;

		if (frame_ < iter->second.begin()->first)
		{
			//result = iter->second.begin()->second;
			result = iter->second.begin()->second.ToEuler();
			return result[id - vIdx];
		}

		if (frame_ > iter->second.rbegin()->first)
		{
			//result = iter->second.rbegin()->second;
			result = iter->second.rbegin()->second.ToEuler();
			return result[id - vIdx];
		}

		const std::map<int, Quaternion>& quaMap = mHermiteQuaternionData[vIdx];

		if (quaMap.size() == 1)
		{
			//result = mHermiteQuaternionData[vIdx].begin()->second;
			result = mHermiteQuaternionData[vIdx].begin()->second.ToEuler();
			return result[id - vIdx];
		}

		size_t leftIndex = 0;
		size_t rightIndex = quaMap.size() > 0 ? quaMap.size() - 1 : 0;
	   
		GetQuaFrameRange(frame_, leftIndex, rightIndex, quaMap);

		std::map<int,Quaternion>::const_iterator leftIter = std::next(quaMap.begin(), leftIndex);
		std::map<int, Quaternion>::const_iterator rightIter = std::next(quaMap.begin(), rightIndex);

		float per_ = (frame_ - (float)leftIter->first) / ((float)rightIter->first - (float)leftIter->first);

		Quaternion lQ = leftIter->second;
		Quaternion rQ = rightIter->second;

		// 四元数插值
		int dx = rightIter->first - leftIter->first;
		Quaternion m1 = mHermiteOutSlopeData[vIdx][leftIter->first] * (float)dx;
		Quaternion m2 = mHermiteInSlopeData[vIdx][rightIter->first] * (float)dx;
		Quaternion q;

		float t2 = per_ * per_;
		float t3 = t2 * per_;

		float a = 2.0F * t3 - 3.0F * t2 + 1.0F;
		float b = t3 - 2.0F * t2 + per_;
		float c = t3 - t2;
		float d = -2.0F * t3 + 3.0F * t2;
		q = lQ * a + m1 * b + m2 * c + rQ * d;
		q.normalise();

		Quaternion resQ;
		if (mHermiteOutSlopeData[vIdx][leftIter->first][0] == std::numeric_limits<float>::infinity() || mHermiteInSlopeData[vIdx][rightIter->first][0] == std::numeric_limits<float>::infinity() ||
			mHermiteOutSlopeData[vIdx][leftIter->first][1] == std::numeric_limits<float>::infinity() || mHermiteInSlopeData[vIdx][rightIter->first][1] == std::numeric_limits<float>::infinity() ||
			mHermiteOutSlopeData[vIdx][leftIter->first][2] == std::numeric_limits<float>::infinity() || mHermiteInSlopeData[vIdx][rightIter->first][2] == std::numeric_limits<float>::infinity() ||
			mHermiteOutSlopeData[vIdx][leftIter->first][3] == std::numeric_limits<float>::infinity() || mHermiteInSlopeData[vIdx][rightIter->first][3] == std::numeric_limits<float>::infinity())
		{
			resQ = lQ;
		}
		else
			resQ = q;

		result = resQ.ToEuler();
		
		return result[id - vIdx];
	}

	void TimelineComponent::CreateTimeLineFile(const char * path)
	{
		if (path == nullptr)
			return;
		std::string filePath = path;
		auto tempInfo = TimeLineInfoManager::instance()->load(Name(filePath));
		if (tempInfo.isNull())
			m_PTimelineInfo = TimeLineInfoManager::instance()->createManual(Echo::Name(filePath));
		else
			m_PTimelineInfo = tempInfo;
		if(m_PTimelineInfo.isNull())
			return;
		resetTimelineInfoData(m_PTimelineInfo);
		m_PTimelineInfo.getPointer()->saveTimelineFile(filePath);
	}
	void TimelineComponent::CreateTimelineMotionFile(const char* path)
	{
		if (path == nullptr )
			return;

		TimelineMotionVec motionVec_;
		for (size_t j =0 ; j < mRootMotionData.idx.size() ; ++j )
		{
			EchoMotionData  motionData_;
		    motionData_.type = mRootMotionData.type[j];

            TimelineMotionMap& motionMap_ = motionData_.motionData;
			int st = mRootMotionData.idx[j].first;
			int en = mRootMotionData.idx[j].second;
			if (en - st != 3)
				continue;
			if (mHermites.size() < static_cast<size_t>(st) || mHermites.size() < static_cast<size_t>(en))
				continue;

			std::set<int>  frameSet_;
			for (int i = st; i < en; ++i)
			{
				for (const auto& iter : mHermites[i]->mKeyMap)
				{
					frameSet_.insert(iter.first);
				}
			}

			for (int i = st; i < en; ++i)
			{
				auto& keyMap_ = mHermites[i]->mKeyMap;
				if (keyMap_.size() == frameSet_.size())
					continue;
				for (int frame_ : frameSet_)
				{
					auto iter = keyMap_.find(frame_);
					if (iter == keyMap_.end())
					{
						float value_ = floatInterpolation(i, static_cast<float>(frame_));
						AddKey(i, frame_, value_);
					}
				}
			}

			bool ProcessSuccess = ProcessRotationSeparately(st);

			if (ProcessSuccess)
				mRootMotionData.type[j] = RootMotionDataType::M_Quaternion;
			motionData_.type = mRootMotionData.type[j];

			if (ProcessSuccess)
			{
				for (int pDataNum = 0; pDataNum < mHermiteQuaternionData[st].size(); ++pDataNum)
				{
					const auto& pQuaternionData = std::next(mHermiteQuaternionData[st].begin(), pDataNum);
					//const auto& pOutSlopeData = std::next(mHermiteOutSlopeData[st].begin(), pDataNum);
					//const auto& pInSlopData = std::next(mHermiteInSlopeData[st].begin(), pDataNum);


					HermiteMotionData motionData;
					int time_ = static_cast<int> (pQuaternionData->first / mDeltaTime * 1000);
					motionMap_.emplace(time_, motionData);

					for (int i = 0; i < 4; ++i)
					{
						motionMap_[time_].value[i] = pQuaternionData->second[i];

						if (mHermiteOutSlopeData[st].find(pQuaternionData->first) != mHermiteOutSlopeData[st].end())
							motionMap_[time_].rightHandTangent[i] = mHermiteOutSlopeData[st][pQuaternionData->first][i] * 0.001f * mDeltaTime;

						if (mHermiteInSlopeData[st].find(pQuaternionData->first) != mHermiteInSlopeData[st].end())
							motionMap_[time_].leftHandTangent[i] = mHermiteInSlopeData[st][pQuaternionData->first][i] * 0.001f * mDeltaTime;
					}
				}
			}

			for (int i = st; i < en; ++i)
			{
				EchoFloatHermite* floatHermite = static_cast<EchoFloatHermite*>(mHermites[i]);
				const std::map<int, Echo::HermiteKey*>& keyMap = floatHermite->mKeyMap;
				for (const auto& keyIter : keyMap)
				{
					HermiteMotionData motionData;
					int time_ = static_cast<int> (keyIter.first / mDeltaTime * 1000);
					motionMap_.emplace(time_, motionData);
					HermiteFloatKey* floatKey = static_cast<HermiteFloatKey*>(keyIter.second);

					motionMap_[time_].isLeftWeighted[i - st] = floatKey->isLeftWeight();
					motionMap_[time_].isRightWeighted[i - st] = floatKey->isRightWeight();

					motionMap_[time_].leftWeight[i - st] = floatKey->leftWeight;
					motionMap_[time_].rightWeight[i - st] = floatKey->rightWeight;

					// 若为四元数插值，已经在此逻辑之前处理了对应数据存储
					if (ProcessSuccess) continue;

					if (i - st == 0)
					{
						motionMap_[time_].value.x = floatKey->value;
						motionMap_[time_].leftHandTangent.x = floatKey->leftHandTangent * mDeltaTime / 1000 ;
						motionMap_[time_].rightHandTangent.x = floatKey->rightHandTangent * mDeltaTime / 1000;
					}
					else if (i - st == 1)
					{
						motionMap_[time_].value.y = floatKey->value;
						motionMap_[time_].leftHandTangent.y = floatKey->leftHandTangent * mDeltaTime / 1000 ;
						motionMap_[time_].rightHandTangent.y = floatKey->rightHandTangent * mDeltaTime / 1000;
					}
					else
					{
						motionMap_[time_].value.z = floatKey->value;
						motionMap_[time_].leftHandTangent.z = floatKey->leftHandTangent * mDeltaTime / 1000 ;
						motionMap_[time_].rightHandTangent.z = floatKey->rightHandTangent * mDeltaTime / 1000 ;
					}
				}
			}

			motionVec_.emplace_back(motionData_);
		}
		
		m_PTimelineInfo->saveTimelineMotionFile(path, motionVec_);
	}
	void TimelineComponent::SetMotionNode(const std::string& comName_)
	{
		if (mComponentToAttributes.find(comName_) == mComponentToAttributes.end())
			return;
		std::vector<attributeData>& attrVec = mComponentToAttributes[comName_];
		bool isExit = false;
		for (const auto& attrIter : attrVec)
		{
			if (attrIter.attributeName == "Position")
			{
				mRootMotionData.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
				mRootMotionData.type.push_back(M_Position);
				isExit = true;
			}
			else if (attrIter.attributeName == "Rotation")
			{
				mRootMotionData.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
				if (attrIter.isQuaternion)
					mRootMotionData.type.push_back(M_Quaternion);
				else
					mRootMotionData.type.push_back(M_Rotation);
				
				isExit = true;
			}
			else if (attrIter.attributeName == "Scale")
			{
				mRootMotionData.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
				mRootMotionData.type.push_back(M_Scale);
				isExit = true;
			}
		}
		if(isExit)
		    mRootMotionData.comName = comName_;
	}
	void TimelineComponent::eraseMotionNode()
	{
		mRootMotionData.idx.clear();
		mRootMotionData.type.clear();
		mRootMotionData.comName = "";
	}
	void TimelineComponent::deleteMotionAttribute(const std::string& attrName_)
	{
		RootMotionDataType type_;
		if (attrName_ == "Position")
			type_ = M_Position;
		else if (attrName_ == "Rotation")
			type_ = M_Rotation;
		else if (attrName_ == "Scale")
			type_ = M_Scale;
		else
			return;
		for(size_t i= 0; i < mRootMotionData.type.size(); ++ i )
		{
			if (mRootMotionData.type[i] == type_)
			{
				mRootMotionData.type.erase(mRootMotionData.type.begin() + i);
				mRootMotionData.idx.erase(mRootMotionData.idx.begin() + i);
				break;
			}
		}
		if (mRootMotionData.type.empty())
			mRootMotionData.comName = "";
	}
	void TimelineComponent::GetTimeLineFile(const char* path, bool synchronous, bool isChangeState_)
	{
		if (path == nullptr)
			return;
		std::string filePath = path;
		if (synchronous)
		{
			m_isCreateFinish = true;
			try
			{
				m_PTimelineInfo = TimeLineInfoManager::instance()->load(Name(filePath));
			}
			catch (...)
			{
				m_PTimelineInfo = TimeLineInfoManager::instance()->createManual(Name(filePath));
				if (m_PTimelineInfo.isNull())
					LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "Timeline :File not exit: " + filePath);
				else
				{
                    m_PTimelineInfo->load();
				}
					
			}
			resetTimelineData(m_PTimelineInfo, isChangeState_);
			setComponentAttribute();
			m_TimelineFilePath = path;
#ifdef ECHO_EDITOR
			if (TimelineName.empty())
			{
			    TimelineName = getComNameToString();
			}
#endif
		}
		else
		{
			ResourceBasePtr pResource = TimeLineInfoManager::instance()->getByName(Name(filePath));
			if (!pResource.isNull())
			{
				if (pResource->isLoaded())
				{
					m_PTimelineInfo = static_cast<TimeLineInfoPtr>(pResource);
					resetTimelineData(m_PTimelineInfo, isChangeState_);
					setComponentAttribute();
					m_TimelineFilePath = path;
					m_isCreateFinish = true;
				}
				else if (pResource->isPrepared())
				{
					pResource->load();
					m_PTimelineInfo = static_cast<TimeLineInfoPtr>(pResource);
					resetTimelineData(m_PTimelineInfo, isChangeState_);
					setComponentAttribute();
					m_TimelineFilePath = path;
					m_isCreateFinish = true;
				}
				else
				{
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
					requestResouce(Name(filePath), TimeLineInfoManager::instance()->getResourceType(), priority);
				}
			}
			else
			{
				ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
				requestResouce(Name(filePath), TimeLineInfoManager::instance()->getResourceType(), priority);
			}
		}
		//setComponentAttribute();
	}

	void TimelineComponent::registerTimelineComListener(TimelineComListener* p_ComListener)
	{
		if (p_ComListener == nullptr)
			return;
		mComListenerVecs.insert(p_ComListener);
	}

	void TimelineComponent::unRegisterTimelineComListener(TimelineComListener* p_ComListener)
	{
		if (p_ComListener == nullptr)
			return;
		mComListenerVecs.erase(p_ComListener);
	}

	void TimelineComponent::setActorId(const std::string& actorName_, int actorId_)
	{
		mActorToId[actorName_] = actorId_;

		const auto& idIter_ = mActorIdData.mActorIdToActor.find(actorId_);
		ActorPtr resultActorPtr_;
		if (idIter_ != mActorIdData.mActorIdToActor.end() && idIter_->second != nullptr)
			resultActorPtr_ = idIter_->second;
		else
		{
			ActorIdDataHandle actorIdData_;
			actorIdData_.mActorIdToActor[actorId_] = ActorPtr();
			ProtocolComponentSystem* pMgr = ProtocolComponentSystem::instance();
			if (pMgr)
			{
				pMgr->callProtocolFunc(PFID_InitTimelineActorId, &actorIdData_, ProtocolHandle(this));
				resultActorPtr_ = actorIdData_.mActorIdToActor[actorId_];
			}
		}
		if (resultActorPtr_ != nullptr)
		{
			initComData(actorName_, resultActorPtr_);
		}
	}

	int TimelineComponent::getActorId(const std::string& actorName_)
	{
		if (mActorToId.find(actorName_) != mActorToId.end())
			return mActorToId[actorName_];
		return -1;
	}

	void TimelineComponent::initActorIdData()
	{
		mActorIdData.mActorIdToActor.clear();
		for (const auto& actorIdIter : mActorToId)
		{
			if(actorIdIter.second != -1)
			    mActorIdData.mActorIdToActor[actorIdIter.second] = ActorPtr();
		}
		ProtocolComponentSystem* pMgr = ProtocolComponentSystem::instance();
		if(pMgr)
		    pMgr->callProtocolFunc(PFID_InitTimelineActorId, &mActorIdData, ProtocolHandle(this));
	}

	void TimelineComponent::resetDataByActorId()
	{
		initActorIdData();
		for (const auto& actorIdIter : mActorToId)
		{
			const auto& idIter_ = mActorIdData.mActorIdToActor.find(actorIdIter.second);
			if (idIter_ == mActorIdData.mActorIdToActor.end() || idIter_->second == nullptr)
				continue;
			initComData(actorIdIter.first, idIter_->second);
		}
	}

	void TimelineComponent::remakeAttributeState()
	{
		size_t sz1 = AttributeName.size();
		size_t sz2 = AttributeToLocation.size();

		if (sz2 != sz1)
		{
			AttributeToLocation.clear();
			for (int i = 0; i < sz1; i++)
			{
				AttributeToLocation.emplace(AttributeName[i], i);
			}
		}
	}
#ifdef ECHO_EDITOR
	void TimelineComponent::remakeCurveState()
	{
		if (showCurveVec.empty())
			return;
		for (auto i = showCurveVec.begin(); i != showCurveVec.end(); i++)
			*i = 0;
	}
#endif

	void TimelineComponent::setAttributeName(const int & idx, const std::string & name)
	{
		if (idx > AttributeToName.size())
			return;
		AttributeToName[idx][1] = name;
	}

	void TimelineComponent::resetTimelineData(TimeLineInfoPtr tInfo,bool isChangePlayState_ , bool isChangePlayWayState_ )
	{
		
		TimelineComponentInfo* TimeLineInfo = static_cast<TimelineComponentInfo*>(m_pActComInfo);
		if (tInfo.isNull())
			return;

		m_PTimelineInfo = tInfo;
		m_TimelineFilePath = tInfo->getName().c_str();
		auto info = tInfo.getPointer();
		if (info == nullptr)
			return;

		mVer = info->mVer;

		if (!TimeLineInfo || !TimeLineInfo->isLoadHermite)
			copyHermiteDatas(info->mHermites, mHermites);
		else
		{
			if(TimeLineInfo)
			    TimeLineInfo->isLoadHermite = false;
		}

        mMaxWeight = info->mMaxWeight;
		if (getType() != todTimelineType_)
		{
			if (!TimeLineInfo || !TimeLineInfo->isLoadDeltaTime)
				mDeltaTime = info->mDeltaTime;
			else
			{
				if (TimeLineInfo)
                   TimeLineInfo->isLoadDeltaTime = false;
			}
			   
			
			if (!TimeLineInfo || !TimeLineInfo->isLoadStartTime)
				startSceneTime = info->startSceneTime;
			else
			{
				if (TimeLineInfo)
					TimeLineInfo->isLoadStartTime = false;
			}

			if (!TimeLineInfo || !TimeLineInfo->isLoadEndTime)
				endSceneTime = info->endSceneTime;
			else
			{
				if (TimeLineInfo)
					TimeLineInfo->isLoadEndTime = false;
			}
		}
#ifdef ECHO_EDITOR
		HermiteNames = info->HermiteNames;
		contentSize_ = info->contentSize_;
		TimelineName = info->TimelineName;

		mCameraFrameToIdx = info->mCameraFrameToIdx;
		mCameraMgrIdx = info->mCameraMgrIdx;
#endif
		if (isChangePlayState_)
		{
			bPlayScene = info->bPlayScene;
		}

		if (isChangePlayWayState_)
		{
			if (!TimeLineInfo || !TimeLineInfo->isLoadPlayWay)
				currentPlayWay = info->currentPlayWay;
			else
			{
				if (TimeLineInfo)
					TimeLineInfo->isLoadPlayWay = false;
			}
		}

		mWidthMultiple = info->mWidthMultiple;
		AttributeName = info->AttributeName;
		AttributeBegin = info->AttributeBegin;
		AttributeEnd = info->AttributeEnd;
		OrderCnt = info->OrderCnt;
		AttributeToName = info->AttributeToName;
		AttributeType = info->AttributeType;
		recordColorTimelineType = info->recordColorTimelineType;
		showComponents = info->showComponents;
		mActorToComponents = info->mActorToComponents;
		mComponentToAttributes = info->mComponentToAttributes;
		mRootMotionData = info->mRootMotionData;
		mActorToId = info->mActorToId;
		initTimelineUIData();
		ProcessRotationSeparately();
		//setComponentAttribute();
	}

	void TimelineComponent::resetTimelineInfoData(TimeLineInfoPtr tInfo)
	{
		if (tInfo.isNull())
			return;
		auto info = tInfo.getPointer();
		if (info == nullptr)
			return;
		copyHermiteDatas(mHermites,info->mHermites);
		info->mMaxWeight = mMaxWeight;
		info->mDeltaTime = mDeltaTime ;
		info->startSceneTime = startSceneTime;
		info->endSceneTime = endSceneTime;

#ifdef ECHO_EDITOR
		info->HermiteNames = HermiteNames;
		info->contentSize_ = contentSize_;
		info->TimelineName = TimelineName;

		info->mCameraFrameToIdx = mCameraFrameToIdx ;
		info->mCameraMgrIdx = mCameraMgrIdx;
#endif
		info->currentPlayWay = currentPlayWay;
		info->bPlayScene = bPlayScene;
		info->mWidthMultiple = mWidthMultiple ;
		info->AttributeName = AttributeName;
		info->AttributeBegin = AttributeBegin;
		info->AttributeEnd = AttributeEnd ;
		info->OrderCnt = OrderCnt;
		info->AttributeToName = AttributeToName;
		info->AttributeType = AttributeType;
		info->recordColorTimelineType = recordColorTimelineType;
		info->showComponents = showComponents;
		info->mActorToComponents = mActorToComponents;
		info->mComponentToAttributes = mComponentToAttributes;
		info->mRootMotionData = mRootMotionData;
		info->mActorToId = mActorToId;
	}

	void TimelineComponent::copyHermiteDatas(const std::vector<EchoHermite*>& oriData_, std::vector<EchoHermite*>& resData_)
	{

		for (auto& hermite : resData_)
		{
			for (auto&hKey : hermite->mKeyMap)
			{
				SAFE_DELETE(hKey.second);
			}
			SAFE_DELETE(hermite);
		}
		resData_.clear();


		for (int i = 0; i < oriData_.size(); i++)
		{
			EchoHermite* hermite = nullptr;
			if (mVer <= 1.21f)
			{
				if (oriData_[i]->mHermiteType == EchoHermite::Float_Hermite)
				{
					hermite = new EchoFloatHermite();
					hermite->mHermiteType = EchoHermite::Float_Hermite;
				}
				else if (oriData_[i]->mHermiteType == EchoHermite::Bool_Hermite)
				{
					hermite = new EchoBoolHermite();
					hermite->mHermiteType = EchoHermite::Bool_Hermite;
				}
				else if (oriData_[i]->mHermiteType == EchoHermite::Fragment_Hermite)
				{
					hermite = new EchoFragmentHermite(static_cast<EchoFragmentHermite*>(oriData_[i])->getFragmentType());
					hermite->mHermiteType = EchoHermite::Fragment_Hermite;
				}
				else if (oriData_[i]->mHermiteType == EchoHermite::String_Hermite)
				{
					hermite = new EchoStringHermite();
					hermite->mHermiteType = EchoHermite::String_Hermite;
				}
			}
			else
			{
				EchoHermiteFactory* factory_ = EchoHermiteFactortManager::getSingleton().getHermiteFactory(oriData_[i]->getType());
				if (!factory_)
					continue;
				hermite = factory_->createHermite();
			}

			for (auto& item : oriData_[i]->mKeyMap)
			{
				/*if (oriData_[i]->mHermiteType == EchoHermite::Float_Hermite)
				{
					HermiteFloatKey* newKey = new HermiteFloatKey;
					key = newKey;
					newKey = nullptr;
				}
				else if (oriData_[i]->mHermiteType == EchoHermite::Bool_Hermite)
				{
					HermiteBoolKey* newKey = new HermiteBoolKey;
					key = newKey;
					newKey = nullptr;
				}
				else if (oriData_[i]->mHermiteType == EchoHermite::Fragment_Hermite)
				{
					HermiteFragmentKey* newKey = new HermiteFragmentKey;
					key = newKey;
					newKey = nullptr;
				}
				else if (oriData_[i]->mHermiteType == EchoHermite::String_Hermite)
				{
					HermiteStringKey* newKey = hermite->CreateKey();
					key = newKey;
					newKey = nullptr;
				}*/
				HermiteKey*  key = hermite->CreateKey();
				item.second->Copy(key);
				assert(key != nullptr);
				hermite->mKeyMap.insert(std::make_pair(item.first, key));
			}
			resData_.emplace_back(hermite);
			hermite = nullptr;
		}

	}




	void TimelineComponent::initComData(const std::string& actName, ActorPtr actPtr)
	{
#ifdef ECHO_EDITOR
			std::string absName_ = actPtr->getCombinedID();
			deleteScenePrefix(absName_);
			mRelNameToAbsName[actName] = absName_;
			mAllActorsMaps[actPtr] = actName;
#endif
		mActorMaps[actName] = actPtr;

		const std::vector<std::string>& comVec_ = mActorToComponents[actName];
		for (const auto& comIter : comVec_)
		{
			initRootMotionData(comIter);
			size_t idx = comIter.find(" : ");
			if (idx == std::string::npos)
				continue;
			idx += 3;
			std::string comTreeName = comIter.substr(idx, comIter.size());
			ActorComponentPtr currCom = nullptr;
			currCom = actPtr->getComponent(comTreeName);

#ifdef ECHO_EDITOR
			if (currCom == nullptr)
			{
				std::string errData = "Component : " + comTreeName;
				errData.append("  is Loss ");
				mErrorInfoVec.push_back(errData);
			}
			//相机关键帧相关
			for (const auto& attrIter : mComponentToAttributes[comIter])
			{
				if (attrIter.attributeName == cameraUseStateName)
				{
					if (!currCom.expired())
						mCameraBaseComMap[currCom] = attrIter.st;
					break;
				}
			}
			addTypeMapItem(comIter, currCom);
#endif
			mComMaps[comIter] = currCom;

			if (!currCom)
				TimeLineInfoLoadingTood::instance()->addMissingTimeline(getActor()->getRootActor(), this);
			//_prepareTimelineResource(comIter, currCom);

		}
	}

	void TimelineComponent::initRootMotionData(const std::string& comName)
	{
		if (!mIsInitRootMotionData && comName == mRootMotionData.comName)
		{
			mIsInitRootMotionData = true;
			for (const auto& attrIter : mComponentToAttributes[comName])
			{
				if (attrIter.attributeName == "Position")
				{
					mRootMotionData.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
					mRootMotionData.type.push_back(M_Position);
				}
				else if (attrIter.attributeName == "Rotation")
				{
					mRootMotionData.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
					mRootMotionData.type.push_back(M_Rotation);
				}
				else if (attrIter.attributeName == "Scale")
				{
					mRootMotionData.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
					mRootMotionData.type.push_back(M_Scale);
				}
			}
		}
	}

	void TimelineComponent::initTimelineUIData()
	{

		if(m_PTimelineInfo ->mVer == 1.1f)
		    dealWithOldData();
		else
            initActorAndComData();
#ifdef ECHO_EDITOR
		showCurveVec.resize(mHermites.size());
		recordComponentStates.clear();
		_reInitFrameToIdx();
#endif  
	}

	void TimelineComponent::initOldData()
	{
		AttributeComponents.resize(AttributeName.size());
		remakeAttributeState();
		bool flag = false;
		if (AttributeToName.empty())
		{
			AttributeToName.resize(AttributeName.size(), std::vector<std::string>(2));
			flag = true;
		}
		for (int i = 0; i < AttributeName.size(); i++)
		{
			std::string  resName = "";
			std::string name = "";
			size_t idx = AttributeName[i].find(" : ");
			if (idx != std::string::npos)
			{
				name = std::string(AttributeName[i].begin(), AttributeName[i].begin() + idx);
				resName = std::string(AttributeName[i].begin() + idx + 3, AttributeName[i].end());
				AttributeComponents[i] = m_pActor->getComponent(name);
				if (flag)
				{
					AttributeToName[i][0] = name;
					AttributeToName[i][1] = resName;
				}
			}
		}
		if (AttributeType.size() != AttributeName.size())
		{
			AttributeType.resize(AttributeName.size(), Default);
		}
		if (showComponents.size() != AttributeName.size())
			showComponents.resize(AttributeName.size(), 1);
	}

	void TimelineComponent::dealWithOldData()
	{
		initOldData();
		auto currActor = getActor();
		String absName = currActor->getCombinedID();
		String relName = currActor->getRelativeNameFromAbsoluteName(absName);
		std::string actorName_ = relName;
		if (mActorToComponents.find(actorName_) != mActorToComponents.end())
			return;
		

#ifdef ECHO_EDITOR
			std::string absNameStr = absName;
			deleteScenePrefix(absNameStr);
			mRelNameToAbsName.insert(std::make_pair(relName, absNameStr));
#endif

		mActorToComponents.insert(std::make_pair(actorName_, std::vector<std::string>()));
		mActorMaps.insert(std::make_pair(actorName_,currActor));

		for (int i = 0; i < AttributeName.size(); i++)
		{
			std::string comName = "";
			if(AttributeComponents[i] != nullptr)
				comName = AttributeComponents[i]->getComNameToString();
			std::string combineName_ = actorName_ + " : " + comName;
			std::string attrName_ = "";

			size_t idx = AttributeName[i].find(" : ");
			if (idx != std::string::npos)
				attrName_ = std::string(AttributeName[i].begin()+idx + 3 , AttributeName[i].end());
			std::string  tex_name = AttributeToName[i][1];
            
			auto comIter = std::find(mActorToComponents[actorName_].begin(), mActorToComponents[actorName_].end(), combineName_);
			if (comIter == mActorToComponents[actorName_].end())
			{
				mActorToComponents[actorName_].emplace_back(combineName_);
				mComponentToAttributes.insert(std::make_pair(combineName_, std::vector<attributeData>()));
#ifdef ECHO_EDITOR
				addTypeMapItem(combineName_, AttributeComponents[i]);
#endif
			}
			mComponentToAttributes[combineName_].emplace_back(AttributeBegin[i],AttributeEnd[i],attrName_,tex_name);
			mComMaps.insert(std::make_pair(combineName_,AttributeComponents[i]));
		}
	}

	void TimelineComponent::initActorAndComData()
	{
		initActorIdData();
		mIsFindListError = false;
		ActorPtr currComActor = getActor();

		for (const auto& actorIter : mActorToComponents)
		{
			PickActorResult  actorData;
			const std::map<int, ActorPtr>& idToActor_ = mActorIdData.mActorIdToActor;

			std::map<std::string, int>::const_iterator idIter  = mActorToId.find(actorIter.first);

			if (idIter != mActorToId.end() && idIter->second != -1 && idToActor_.contains(idIter->second))
			{
				actorData.seleteActor = idToActor_.at(idIter->second);
				if (actorData.seleteActor == nullptr)
				{
                    mIsFindListError = true;
					std::string logInfo = "Timeline : Find List find Actor  Error; Name is ";
					logInfo.append(getActor()->getCombinedID()).append(" : ").append(getComNameToString()).append("ID is :").append(std::to_string(idIter->second));
					LogManager::getSingleton().logMessage(Echo::LML_NORMAL, logInfo.data());
				}
			}
			else 
			{
				if (actorIter.first == "%%")
					actorData.seleteActor = currComActor;
				else
	                actorData = currComActor->getActorComByRelativeName(actorIter.first,false);
			}
			if (actorData.seleteActor == nullptr)
			{
				TimeLineInfoLoadingTood::instance()->addMissingTimeline(getActor()->getRootActor(),this);
				if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				{
					//%% 表示当前actor；多一个%表示多往上一级；$$表示下一级
					std::string errData = "Actor : " + actorIter.first;
					errData.append(" is loss");
					mErrorInfoVec.push_back(errData);
				}
				mActorMaps[actorIter.first] = nullptr;
				continue;
			}
			initComData(actorIter.first, actorData.seleteActor);
		}
		mIsInitRootMotionData = false;

		//资源收集相关
		const ActorComponentPtr& cutSceneCom_ = m_pActor->getCutSceneCom();
		if (!cutSceneCom_.expired())
		{
			static_cast<AnimationTimelineComponent*>(cutSceneCom_.get())->addToTimelineList(weak_from_this());
		}
	}

	void TimelineComponent::deleteScenePrefix(std::string& absName)
	{
		size_t idx = absName.find("$");
		size_t prefix_idx = absName.find("#");
		if (prefix_idx != absName.npos && idx != absName.npos)
			absName = absName.substr(idx + 2, absName.size());
	}

	void TimelineComponent::_getOldKey(HermiteKey** oldKey_, int idx, int frame)
	{
		if (idx > mHermites.size())
			return;
		if (mHermites[idx]->mKeyMap.count(frame) == 0)
			return;

		*oldKey_ = mHermites[idx]->CreateKey();

		mHermites[idx]->mKeyMap[frame]->Copy((*oldKey_));
	}

	void TimelineComponent::_addKey(HermiteKey* oldKey, int idx, int frame, int diffFrame, const float& diffValue)
	{
		if (idx > mHermites.size() || oldKey == nullptr)
			return;

		HermiteKey* newKey = mHermites[idx]->CreateKey();

		oldKey->Copy(newKey);
       
		if (mHermites[idx]->mHermiteType == EchoHermite::Float_Hermite)
			static_cast<HermiteFloatKey*>(newKey)->value -= diffValue;
		int newT = std::max(frame + diffFrame, 0);

		if (mHermites[idx]->mKeyMap.count(newT) != 0)
		{
			DeleteKey(idx, newT);
		}

		mHermites[idx]->mKeyMap.emplace(newT, newKey);
		mMaxWeight = std::max(mMaxWeight, mHermites[idx]->GetMaxLength());
		newKey = nullptr;

#ifdef ECHO_EDITOR
		addKeySetData(idx, newT);
#endif
	}

	void TimelineComponent::_dealWithIdxRemove(int st,int en)
	{
#ifdef ECHO_EDITOR
		//处理索引关系
		int size = en - st;

		for (auto& comIter : mComponentToAttributes)
		{
			for (auto& attrIter : comIter.second)
			{
				//处理属性记录的索引关系
				if (attrIter.st > st)
				{
					if (attrIter.st == mCameraActiveIdx)
						mCameraActiveIdx -= size;
					attrIter.st -= size;
					attrIter.en -= size;
				}
				//如果是相机的启用标签,则处理下相关的相机索引容器
				if (attrIter.attributeName == cameraUseStateName)
				{
					if (!mComMaps[comIter.first].expired() && mCameraBaseComMap.count(mComMaps[comIter.first]) != 0)
						mCameraBaseComMap[mComMaps[comIter.first]] = attrIter.st;
				}
			}
		}

		for (auto& iter : mCameraFrameToIdx)
		{
			if (iter.second >= st)
				iter.second -= size;
		}


		dealWithRootMotionIdx(st, size);

		if (mCameraMgrIdx > st)
			mCameraMgrIdx -= size;
#endif
	}

	void TimelineComponent::_dealWithIdxExtend(int st, int en)
	{
#ifdef ECHO_EDITOR
		for (auto& comIter : mComponentToAttributes)
		{
			for (auto& attrIter : comIter.second)
			{
				//处理属性记录的索引关系
				if (attrIter.st >= en)
				{
					if (attrIter.st == mCameraActiveIdx)
						mCameraActiveIdx += 1;
					attrIter.st += 1;
					attrIter.en += 1;
				}
				//如果是相机的启用标签,则处理下相关的相机索引容器
				if (attrIter.attributeName == cameraUseStateName)
				{
					if (!mComMaps[comIter.first].expired() && mCameraBaseComMap.count(mComMaps[comIter.first]) != 0)
						mCameraBaseComMap[mComMaps[comIter.first]] = attrIter.st;
				}
			}
		}

		for (auto& iter : mCameraFrameToIdx)
		{
			if (iter.second >= en)
				iter.second += 1;
		}
		if (mCameraMgrIdx >= en)
			mCameraMgrIdx += 1;

		dealWithRootMotionIdxAdd(en);
#endif

	}

	void TimelineComponent::_reInitFrameToIdx()
	{
#ifdef ECHO_EDITOR
		resetKeySetData();
		resetIdxToAttr();
		initIdxToAttr();
		initKeySetData();
#endif
	}

	Quaternion ToQuaternion(Vector3 Euler)
	{
		/*Vector3 someEulerAngles = Euler * Deg2Rad(1.f);

		float cX(cos(someEulerAngles.x / 2.0f));
		float sX(sin(someEulerAngles.x / 2.0f));

		float cY(cos(someEulerAngles.y / 2.0f));
		float sY(sin(someEulerAngles.y / 2.0f));

		float cZ(cos(someEulerAngles.z / 2.0f));
		float sZ(sin(someEulerAngles.z / 2.0f));

		Quaternion qX(sX, 0.0F, 0.0F, cX);
		Quaternion qY(0.0F, sY, 0.0F, cY);
		Quaternion qZ(0.0F, 0.0F, sZ, cZ);

		Quaternion q = (qY * qX) * qZ;*/

		Quaternion q;
		q.FromEuler(Euler);

		return q;
	}

	void TimelineComponent::insertQuaternionData(int vIdx, int frame)
	{
		std::map<int, Quaternion>* inertMap = nullptr;

		auto iter = mHermiteQuaternionData.find(vIdx);
		if (iter != mHermiteQuaternionData.end())
			inertMap = &iter->second;
		else
			inertMap = &mHermiteQuaternionData[vIdx];

		if (judgeIsExitKeyFrame(vIdx, frame) ||
			judgeIsExitKeyFrame(vIdx + 1, frame) ||
			judgeIsExitKeyFrame(vIdx + 2, frame))
		{
			Vector3 eguAngle_;
			eguAngle_.x = floatInterpolation(vIdx, float(frame));
			eguAngle_.y = floatInterpolation(vIdx + 1, float(frame));
			eguAngle_.z = floatInterpolation(vIdx + 2, float(frame));

			Quaternion q = ToQuaternion(eguAngle_);
			q.normalise();

			(*inertMap)[frame] = q;
		}
		else
		{	// 清理无效数据
			inertMap->erase(frame);
		}
	}

	void TimelineComponent::resourceLoadFinish(ProtocolHandle handle_)
	{
		m_LoadTimelineObjectSet.erase(handle_);

		const ActorComponentPtr& cutSceneCom_ = m_pActor->getCutSceneCom();
		if (!cutSceneCom_.expired())
			static_cast<AnimationTimelineComponent*>(cutSceneCom_.get())->notifyTimelineLoadPercent(weak_from_this());

		if(m_LoadTimelineObjectSet.empty())
			_loadFinish();
	}

	int TimelineComponent::preLoadResource(ProtocolHandle handle)
	{
		for (const comMap::value_type& comIter : mComMaps)
		{
			if (comIter.second.expired())
				continue;
			_prepareTimelineResource(comIter.first,comIter.second);
		}
		return m_LoadObjectCount;
	}

	void TimelineComponent::setLoadTimeOutTime(float time_)
	{
		m_LoadTimeoutData = time_;
	}

	void TimelineComponent::_loadFinish()
	{
		if (m_LoadTimelineObjectSet.empty())
		{
			m_isCreateFinish = true;
			m_LoadObjectTickSecond = 0.f;

			if (!m_pActor.expired())
				m_pActor->notifyComponentFinish(weak_from_this());
		}
	}


#ifndef  Collect_Hermite_Key_Resource
#define  Collect_Hermite_Key_Resource(HermiteType,resourceVec,resourceType)\
{\
	    HermiteType* key_ = static_cast<HermiteType*>(keyIter.second);\
		if (!key_)\
		{\
			assert(false && "HermiteKey is nullptr");\
			continue;\
		}\
		resourceVec.resourceType.emplace_back(key_->value);\
}
#endif // !

	void TimelineComponent::_prepareTimelineResource(const std::string& comName, const ActorComponentPtr& comPtr)
	{
		if (!m_bIsCollectResource || comPtr.expired())
			return;
		
		comPtr->setTimelineCom(this);
		//收集资源
		CollectResourceData collectData;
		bool isCollect_ = false;
		for (const auto& attrIter : mComponentToAttributes[comName])
		{
			if (!comPtr->judgeIsTimelinePreLoadObj(attrIter.attributeName))
				continue;

			for (int i = attrIter.st; i < attrIter.en; ++i)
			{
				if (i >= mHermites.size())
					continue;
				if (!mHermites[i])
				{
					assert(false && "hermite is nullptr");
					continue;
				}
				bool isCollectExtraResource = false;
				for (const std::map<int, HermiteKey*>::value_type& keyIter : mHermites[i]->mKeyMap)
				{
					switch (mHermites[i]->mHermiteType)
					{
					case EchoHermite::Float_Hermite:
					{
						Collect_Hermite_Key_Resource(HermiteFloatKey, collectData, m_IdResourceVec);
						if (mHermites[i]->getType() != EchoFloatHermite::mType)
							isCollectExtraResource = true;
						break;
					}
					case EchoHermite::Bool_Hermite:
					{
						if (mHermites[i]->getType() != EchoBoolHermite::mType)
							isCollectExtraResource = true;
						break;
					}
					case EchoHermite::String_Hermite:
					{
						Collect_Hermite_Key_Resource(HermiteStringKey, collectData, m_StrResourceVec);

						if (mHermites[i]->getType() != EchoStringHermite::mType)
							isCollectExtraResource = true;
						break;
					}
					case EchoHermite::Fragment_Hermite:
					{
						if (mHermites[i]->getType() != EchoFragmentHermite::mType)
							isCollectExtraResource = true;
						break;
					}
					default:
						break;
					}
				}

				if(isCollectExtraResource)
				  comPtr->preLoadTimelineResource(&mHermites[i]->mKeyMap, TimelineResourceType::TL_Type_Key, isCollect_);
			}
		}

		if (!collectData.m_IdResourceVec.empty() || !collectData.m_StrResourceVec.empty() || isCollect_)
		{
			comPtr->preLoadTimelineResource(&collectData, TimelineResourceType::TL_Type_Resource, isCollect_);
			m_LoadTimelineObjectSet.insert(ProtocolHandle(comPtr.get()));
			++m_LoadObjectCount;
		}
		
	}

	bool TimelineComponent::ProcessRotationSeparately(int id, bool remove)
	{
		//LogManager::instance()->logMessage(LML_NORMAL, "----------[TimelineComponent::ProcessRotationSeparately] Play id : " + std::to_string(id));

		std::vector<int> pIdxs;	// 存储需要转换为四元数的Attribute数据的st值

		if (id == -1)
		{	// 判断当前是否为旋转属性，并寻找st值
			for (auto& comToAttr : mComponentToAttributes)
			{
				std::vector<attributeData>& data = comToAttr.second;
				auto iter = std::find_if(data.begin(), data.end(), [&](attributeData& d) {
					return d.attributeName == rotationName_ && d.isQuaternion;
					});

				if (iter == data.end()) continue;
				if (iter->st >= mHermites.size()) continue;

				pIdxs.push_back(iter->st);
			}

			mHermiteQuaternionData.clear();
			mHermiteOutSlopeData.clear();
			mHermiteInSlopeData.clear();
		}
		else
		{
			// 判断当前是否为旋转属性，并寻找st值
			int pIdx = id;
			bool isRotation = false;
			for (auto& comToAttr : mComponentToAttributes)
			{
				std::vector<attributeData>& data = comToAttr.second;
				auto iter = std::find_if(data.begin(), data.end(), [&](attributeData& d) {
					return d.st <= id && d.en > id && d.attributeName == rotationName_ && d.isQuaternion;
					});

				if (iter == data.end()) continue;
				if (iter->st >= mHermites.size()) continue;

				pIdx = iter->st;
				isRotation = true;
				break;
			}
			//清理已存在的对应st的四元数数据
			if (mHermiteQuaternionData.count(pIdx) != 0)
			{
				mHermiteQuaternionData[pIdx].clear();
				mHermiteOutSlopeData[pIdx].clear();
				mHermiteInSlopeData[pIdx].clear();
			}

			if (isRotation)
				pIdxs.push_back(pIdx);
		}

		// 计算当前st值下，当前帧数t下的关键点左右斜率的四元数
		auto GetQuaternionFormCurve = [&](int pId, int preFrame, int nextFrame, float t)
		{
			Quaternion result;

			EchoFloatHermite* pHermite_x = static_cast<EchoFloatHermite*>(mHermites[pId]);
			EchoFloatHermite* pHermite_y = static_cast<EchoFloatHermite*>(mHermites[pId + 1]);
			EchoFloatHermite* pHermite_z = static_cast<EchoFloatHermite*>(mHermites[pId + 2]);
			if (!pHermite_x || !pHermite_y || !pHermite_z)
			{
				assert(!"Error");
				return result;
			}

			Vector3 euler;
			euler.x = pHermite_x->Interpolate(t, preFrame, nextFrame);
			euler.y = pHermite_y->Interpolate(t, preFrame, nextFrame);
			euler.z = pHermite_z->Interpolate(t, preFrame, nextFrame);

			result = ToQuaternion(euler);

			return result;
		};

		for (int j = 0; !remove && j < pIdxs.size(); ++j)
		{
			int pIdx = pIdxs[j];

#if 0
		// 仅能在编辑器使用
		auto ranges_view = mAttrToFrameSet | std::views::filter([&](auto& a) {
				size_t ps = a.first.rfind(":") + 2;
			String value = a.first.substr(ps, a.first.size() - ps);
			return value == rotationName_.c_str();
		}) | std::views::values;

			for (std::map<int, std::set<int>>& frame_idx : ranges_view)
			{
				for (std::pair<const int, std::set<int>>& pair_ : frame_idx)
					insertQuaternionData(pIdx, pair_.first);
			}
#endif // 0

#if 1
			for (auto& comToAttr : mComponentToAttributes)
			{
				std::vector<attributeData>& attrData = comToAttr.second;
				for (attributeData& attr : attrData/* | std::views::filter(
					[&](attributeData& attribute) {return attribute.attributeName == rotationName_ && attribute.st == pIdx;})*/
					)
				{
					if (attr.attributeName == rotationName_ && attr.st == pIdx)
					{
						std::set<int> p_frames;
						for (int hermite_id = attr.st; hermite_id < attr.en; ++hermite_id)
							for (const auto& frame_d : mHermites[hermite_id]->mKeyMap/* | std::views::keys*/)
								p_frames.insert(frame_d.first);

						for (int frame : p_frames)
							insertQuaternionData(attr.st, frame);
					}
				}
			}
#endif // 0

			std::map<int, Quaternion>& outSlope = mHermiteOutSlopeData[pIdx];
			std::map<int, Quaternion>& inSlope = mHermiteInSlopeData[pIdx];
			std::map<int, Quaternion>& quaMap = mHermiteQuaternionData[pIdx];
			// 计算当前四元数之间左右斜率的四元数
			for (int i = 0; i < std::max(0, (int)quaMap.size() - 1); ++i)
			{
				std::pair<const int, Quaternion> element = *std::next(quaMap.begin(), i);
				std::pair<const int, Quaternion> next_element = *std::next(quaMap.begin(), i + 1);

				float cuT = static_cast<float>(element.first) * 0.999f + static_cast<float>(next_element.first) * 0.001f;
				Quaternion quat = GetQuaternionFormCurve(pIdx, element.first, next_element.first, cuT);
				Quaternion qDelta = (quat - element.second) * (1000.f / (next_element.first - element.first));
				outSlope[element.first] = qDelta;

				cuT = static_cast<float>(element.first) * 0.001f + static_cast<float>(next_element.first) * 0.999f;
				quat = GetQuaternionFormCurve(pIdx, element.first, next_element.first, cuT);
				Quaternion qDelta2 = (next_element.second - quat) * (1000.f / (next_element.first - element.first));
				inSlope[next_element.first] = qDelta2;
			}

			if (quaMap.empty()) continue;

			// 将相邻四元数之间与首位四元数之间的大角改为小角
			std::pair<int, Quaternion> last_element = *quaMap.rbegin();
			for (int i = 0; i < quaMap.size(); ++i)
			{
				std::pair<const int, Quaternion> element = *std::next(quaMap.begin(), i);
				int index = element.first;

				if (last_element.second.Dot(element.second) < 0.0f)
				{
					element.second = -element.second;
					quaMap[index] = element.second;
					if (outSlope.find(index) != outSlope.end())
						outSlope[index] = -outSlope[index];

					if (inSlope.find(index) != inSlope.end())
						inSlope[index] = -inSlope[index];
				}
				last_element = element;
			}
		}

		return !pIdxs.empty();
	}

	void TimelineComponent::resourceLoadComplete(ProtocolHandle protolHandle_)
	{
	}

	void TimelineComponent::OutputDebuggingInformation()
	{
		stringstream ss;
		for (std::pair<const int, std::map<int, Quaternion>>& data : mHermiteQuaternionData)
		{
			ss << data.first << ": \n";
			for (std::pair<const int, Quaternion>& frame_data : data.second)
			{
				float time = frame_data.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << frame_data.second.x << "\t"
					<< frame_data.second.y << "\t"
					<< frame_data.second.z << "\t"
					<< frame_data.second.w << "\n";
			}
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::mHermiteQuaternionData] Set Quaternion : \n" + ss.str());
		ss.str("");

		for (std::pair<const int, std::map<int, Quaternion>>& data : mHermiteOutSlopeData)
		{
			ss << data.first << ": \n";
			for (std::pair<const int, Quaternion>& frame_data : data.second)
			{
				float time = frame_data.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << frame_data.second.x << "\t"
					<< frame_data.second.y << "\t"
					<< frame_data.second.z << "\t"
					<< frame_data.second.w << "\n";
			}
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::mHermiteOutSlopeData] Set Out Quaternion : \n" + ss.str());
		ss.str("");

		for (std::pair<const int, std::map<int, Quaternion>>& data : mHermiteInSlopeData)
		{
			ss << data.first << ": \n";
			for (std::pair<const int, Quaternion>& frame_data : data.second)
			{
				float time = frame_data.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << frame_data.second.x << "\t"
					<< frame_data.second.y << "\t"
					<< frame_data.second.z << "\t"
					<< frame_data.second.w << "\n";
			}
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::mHermiteInSlopeData] Set In Quaternion : \n" + ss.str());
		ss.str("");

		std::vector<int> pQuaternionIdxs;
		std::vector<int> pOtherIdxs;

		Vector3 placeholder;

		for (const auto& iter : mComponentToAttributes)
		{
			for (const attributeData& attrData : iter.second)
			{
				if (attrData.attributeName == rotationName_ && attrData.isQuaternion)
					pQuaternionIdxs.push_back(static_cast<int>(attrData.st));
				else
					pOtherIdxs.push_back(static_cast<int>(attrData.st));
			}
		}

		int frameBegin = static_cast<int>(startSceneTime * mDeltaTime);
		int frameEnd = static_cast<int>(endSceneTime * mDeltaTime);

		for (int id : pQuaternionIdxs)
		{
			ss << " \nid : " << id << " ------- " << "\n";
			for (int frame = frameBegin; frame < frameEnd; ++frame)
			{
				QuaternionInterPolate(id, (float)frame, placeholder);

				float time = frame / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << frame / mDeltaTime * 1000 << " : " << (two ? "\t\t" : "\t");
				ss << placeholder.x << "\t"
					<< placeholder.y << "\t"
					<< placeholder.z << "\t";
					//<< placeholder.w << "\n";
			}
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::QuaternionInterPolate] : \n" + ss.str());
		ss.str("");

		// shuc TODO

		for (int id : pOtherIdxs)
		{
			EchoFloatHermite* hermite_x = static_cast<EchoFloatHermite*>(mHermites[id]);
			EchoFloatHermite* hermite_y = static_cast<EchoFloatHermite*>(mHermites[id + 1]);
			EchoFloatHermite* hermite_z = static_cast<EchoFloatHermite*>(mHermites[id + 2]);

			for (const auto& key : hermite_x->mKeyMap)
			{
				float time = key.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << reinterpret_cast<HermiteFloatKey*>(key.second)->value << "\n";
			}
			ss << "\n";

			for (const auto& key : hermite_y->mKeyMap)
			{
				float time = key.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << reinterpret_cast<HermiteFloatKey*>(key.second)->value << "\n";
			}
			ss << "\n";

			for (const auto& key : hermite_z->mKeyMap)
			{
				float time = key.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << reinterpret_cast<HermiteFloatKey*>(key.second)->value << "\n";
			}
			ss << "\n";
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::HermiteValue] : \n" + ss.str());
		ss.str("");

		for (int id : pOtherIdxs)
		{
			EchoFloatHermite* hermite_x = static_cast<EchoFloatHermite*>(mHermites[id]);
			EchoFloatHermite* hermite_y = static_cast<EchoFloatHermite*>(mHermites[id + 1]);
			EchoFloatHermite* hermite_z = static_cast<EchoFloatHermite*>(mHermites[id + 2]);

			for (const auto& key : hermite_x->mKeyMap)
			{
				float time = key.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << key.second->leftHandTangent * mDeltaTime / 1000 << "\t" << key.second->leftWeight << "\t";
				ss << key.second->rightHandTangent * mDeltaTime / 1000 << "\t" << key.second->rightWeight << "\n";
			}
			ss << "\n";

			for (const auto& key : hermite_y->mKeyMap)
			{
				float time = key.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << key.second->leftHandTangent * mDeltaTime / 1000 << "\t" << key.second->leftWeight << "\t";
				ss << key.second->rightHandTangent * mDeltaTime / 1000 << "\t" << key.second->rightWeight << "\n";
			}
			ss << "\n";

			for (const auto& key : hermite_z->mKeyMap)
			{
				float time = key.first / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << time << " : " << (two ? "\t\t" : "\t");
				ss << key.second->leftHandTangent * mDeltaTime / 1000 << "\t" << key.second->leftWeight << "\t";
				ss << key.second->rightHandTangent * mDeltaTime / 1000 << "\t" << key.second->rightWeight << "\n";
			}
			ss << "\n";
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::HermiteTangent Weight] : \n" + ss.str());
		ss.str("");

		for (int id : pOtherIdxs)
		{
			ss << " \nid : " << id << " ------- " << "\n";

			EchoFloatHermite* hermite_x = static_cast<EchoFloatHermite*>(mHermites[id]);
			EchoFloatHermite* hermite_y = static_cast<EchoFloatHermite*>(mHermites[id + 1]);
			EchoFloatHermite* hermite_z = static_cast<EchoFloatHermite*>(mHermites[id + 2]);
			for (int frame = frameBegin; frame < frameEnd; ++frame)
			{
				float pointValue_x = hermite_x->Interpolate((float)frame);
				float pointValue_y = hermite_y->Interpolate((float)frame);
				float pointValue_z = hermite_z->Interpolate((float)frame);

				float time = frame / mDeltaTime * 1000;
				bool two = time - static_cast<int>(time) == 0;
				ss << frame / mDeltaTime * 1000 << " : " << (two ? "\t\t" : "\t");
				ss << pointValue_x << "\t"
					<< pointValue_y << "\t"
					<< pointValue_z << "\n";
			}
		}

		LogManager::instance()->logMessage(LML_NORMAL, "[TimelineComponent::InterPolate] : \n" + ss.str());
		ss.str("");
	}

	void TimelineComponent::shutDownTimelineData()
	{
		for (int i = 0; i < AttributeComponents.size(); ++i)
			AttributeComponents[i] = nullptr;
		for (auto& hermite : mHermites)
		{
			for (auto& hKey : hermite->mKeyMap)
			{
				SAFE_DELETE(hKey.second);
			}
			SAFE_DELETE(hermite);
		}
		mHermites.clear();
		mActorToComponents.clear();
		mComponentToAttributes.clear();
		m_TimelineFilePath = "";
		bPlayScene = false;

		mCameraActiveFrame = 0;
	    mCameraActiveIdx = -1;
		OrderCnt = 0;

		for (const comMap::value_type& comIter : mComMaps)
		{
			if (!comIter.second.expired())
				comIter.second->setTimelineCom(nullptr);
		}
		mComMaps.clear();

		m_LoadTimelineObjectSet.clear();
		m_LoadObjectTickSecond = 0.f;

		mActorMaps.clear();
		m_PTimelineInfo.setNull();
#ifdef ECHO_EDITOR
		mSelectAttributeData.reset();
		mCameraActiveFrame = -1;
		mCameraBaseComMap.clear();
		mCameraFrameToIdx.clear();
		mCameraMgrIdx = -1;
		resetKeySetData();
		mIdxToAttr.clear();
#endif
	}

#ifdef ECHO_EDITOR
	void TimelineComponent::addTypeMapItem(const std::string& comName, EchoWeakPtr<ActorComponent> com)
	{
		if (com == nullptr)
			return;

		const Name& comType = com->getType();
		const std::string comTypeName = mComTypeMaps[comType];
		mComTypeMappingMaps[comName] = comTypeName;
		if (mTypeMap.find(comTypeName) == mTypeMap.end())
		{
			std::pair<bool, int> temp = std::make_pair(true, 1);
			mTypeMap.insert(std::make_pair(comTypeName, temp));
		}
		else
		{
			mTypeMap[comTypeName].second++;
		}
	}
	void TimelineComponent::createAttributeMapping(const std::string& actName_, const std::string& combineName_, const std::string& attributeName_, const std::string& tex_name, const int& size_,attributeType attrType)
	{
		mActorToId[actName_] = -1;
		if (mActorToComponents.find(actName_) == mActorToComponents.end())
		{
			mActorToComponents.insert(std::make_pair(actName_, std::vector<std::string>()));
			mActorToComponents[actName_].emplace_back(combineName_);
			mComponentToAttributes.insert(std::make_pair(combineName_, std::vector<attributeData>()));
		} 
		if (mComponentToAttributes.find(combineName_) == mComponentToAttributes.end())
		{
			mActorToComponents[actName_].emplace_back(combineName_);
			mComponentToAttributes.insert(std::make_pair(combineName_, std::vector<attributeData>()));
		}
		addRootMotionIdx(combineName_, attributeName_, size_);

		int st = OrderCnt;
		OrderCnt += size_;

		mIdxToAttr[st] = std::make_pair(combineName_ + " : " + attributeName_,OrderCnt);

		if (attributeName_.find("Color") != std::string::npos && size_ == 4)
			attrType.isColor = 1;
		if (attributeName_ == "cameraComUseState")
			attrType.isCamera = 1;
		mComponentToAttributes[combineName_].emplace_back(st, OrderCnt, attributeName_, tex_name,true, attrType.isColor, attrType.isCamera, attrType.isNoInterpolate, attrType.isAnimation, attrType.isTruncation, attrType.isQuaternion);
          
	}
	int TimelineComponent::deleteAttributeMapping(const std::string& actName_, const std::string& combineName_, const std::string& attributeName_)
	{
		auto iter = std::find_if(mComponentToAttributes[combineName_].begin(), mComponentToAttributes[combineName_].end(),
			[&attributeName_](const attributeData& lhs) {
				return lhs.attributeName == Name(attributeName_);
			});
		if (iter == mComponentToAttributes[combineName_].end())
			return -1;

		bool isEmpty1 = mCameraBaseComMap.empty();
		if (attributeName_ == cameraUseStateName.c_str() && mCameraBaseComMap.count(mComMaps[combineName_]) != 0)
		{
			deleteCameraBaseComData(mComMaps[combineName_]);
		}
		bool isEmpty2 = mCameraBaseComMap.empty();

		int resultId = iter->st;
		RemoveGroupProperty(iter->st, iter->en);
		int size = iter->en - iter->st;
		if (isEmpty1 != isEmpty2)
			size += 1;

		_dealWithIdxRemove(iter->st, iter->st + size);
	    
		//comMapping容器移除相关属性
		mComponentToAttributes[combineName_].erase(iter);
		if (mComponentToAttributes[combineName_].empty())
		{
			if (mComMaps.count(combineName_) != 0 && !mComMaps[combineName_].expired())
				mComMaps[combineName_]->setTimelineCom(nullptr);

			mComMaps.erase(combineName_);

			mComponentToAttributes.erase(combineName_);

			//属性过滤器相关
			std::string typeName = mComTypeMappingMaps[combineName_];
			mComTypeMappingMaps.erase(combineName_);
			mTypeMap[typeName].second--;
			if (mTypeMap[typeName].second == 0)
				mTypeMap.erase(typeName);

			//处理actor ID ,如果改Actor已经没有下属组件，则让该actor从Actor容器中移除
			auto actIter = std::find(mActorToComponents[actName_].begin(), mActorToComponents[actName_].end(), combineName_);
			mActorToComponents[actName_].erase(actIter);
			if (mActorToComponents[actName_].empty())
			{
				if (mActorToId.find(actName_) != mActorToId.end())
					mActorToId.erase(actName_);
				mActorToComponents.erase(actName_);
				mActorMaps.erase(actName_);
			}

		}
		OrderCnt -= size;

		_reInitFrameToIdx();

		return resultId;
	}
	void TimelineComponent::addExtraAttribue(const std::string& combineName_, const std::string& attributeName_)
	{
		//判断是否能找到该属性，找不到就返回

		if (!mComponentToAttributes.contains(combineName_))
			return;
		std::vector<attributeData>::iterator iter = std::find_if(mComponentToAttributes[combineName_].begin(), mComponentToAttributes[combineName_].end(),
		[&attributeName_](const attributeData& lhs) {
			return lhs.attributeName == Name(attributeName_);
		});
		if (iter == mComponentToAttributes[combineName_].end())
			return;

		if (mHermites.size() <= iter->st  || !mHermites[iter->st])
		{
			assert(!"this Attribute Idx is Error");
			return;
		}

		if (EchoHermiteFactortManager::getSingleton().getHermiteFactory(mHermites[iter->st]->getType()) == nullptr)
			assert(!"this hermitekey is not register");
		EchoHermite*   hermite = EchoHermiteFactortManager::getSingleton().getHermiteFactory(mHermites[iter->st]->getType())->createHermite();
		
		_dealWithIdxExtend(iter->st , iter->en);

		mHermites.resize(mHermites.size() + 1);
		HermiteNames.resize(mHermites.size() + 1);
		showCurveVec.resize(mHermites.size() + 1);

		if (iter->en < mHermites.size())
		{
			// 全体关键帧后移操作
			for (int i = static_cast<int>(mHermites.size()) - 1; i > iter->en; --i)
			{
				mHermites[i] = mHermites[i - 1];
				HermiteNames[i] = HermiteNames[i - 1];
				showCurveVec[i] = showCurveVec[i - 1];
			}
			mHermites[iter->en] = hermite;
			HermiteNames[iter->en] = HermiteNames[iter->st];
		}

		iter->en = iter->en + 1;
		++OrderCnt;

		_reInitFrameToIdx();
	}
	void TimelineComponent::deleteExtraAttribute(const std::string& combineName_, const std::string& attributeName_, int idx)
	{
		if (!mComponentToAttributes.contains(combineName_))
			return;
		std::vector<attributeData>::iterator iter = std::find_if(mComponentToAttributes[combineName_].begin(), mComponentToAttributes[combineName_].end(),
			[&attributeName_](const attributeData& lhs) {
				return lhs.attributeName == Name(attributeName_);
			});
		if (iter == mComponentToAttributes[combineName_].end())
			return;

		if (mHermites.size() <= idx || !mHermites[idx])
		{
			assert(!"this Attribute  extraIdx is Error");
			return;
		}

		//释放内存
		RemoveProperty(idx);

		_dealWithIdxRemove(idx, idx + 1);

		iter->en = iter->en - 1;

		--OrderCnt;
		_reInitFrameToIdx();
	}
	void TimelineComponent::insertExtraAttribute(
		const std::string& actName_, const std::string& combineName_, const std::string& attributeName_, 
		const std::string& tex_name, const int& size_, attributeType attrType, 
		const EchoHermite::HermiteType& type, const Name& typeName, const FragmentType& type_,
		int num)
	{
		mActorToId[actName_] = -1;
		if (mActorToComponents.find(actName_) == mActorToComponents.end())
		{
			mActorToComponents.insert(std::make_pair(actName_, std::vector<std::string>()));
			mActorToComponents[actName_].emplace_back(combineName_);
			mComponentToAttributes.insert(std::make_pair(combineName_, std::vector<attributeData>()));
		}
		if (mComponentToAttributes.find(combineName_) == mComponentToAttributes.end())
		{
			mActorToComponents[actName_].emplace_back(combineName_);
			mComponentToAttributes.insert(std::make_pair(combineName_, std::vector<attributeData>()));
		}

		if (combineName_ == mRootMotionData.comName)
		{
			mRootMotionData.idx.emplace_back(std::make_pair(OrderCnt, OrderCnt + size_));

			if (attributeName_ == positionName_.c_str())
				mRootMotionData.type.emplace_back(RootMotionDataType::M_Position);
			else if (attributeName_ == rotationName_.c_str())
				mRootMotionData.type.emplace_back(RootMotionDataType::M_Rotation);
			else if (attributeName_ == scaleName_.c_str())
				mRootMotionData.type.emplace_back(RootMotionDataType::M_Scale);
		}

		OrderCnt += size_;

		for (auto& iter : mComponentToAttributes)
			for (auto& data : iter.second)
				if (data.st >= num)
				{
					data.st += size_;
					data.en += size_;
				}
		
		if (attributeName_.find("Color") != std::string::npos && size_ == 4)
			attrType.isColor = 1;
		if (attributeName_ == "cameraComUseState")
			attrType.isCamera = 1;
		mComponentToAttributes[combineName_].emplace_back(num, num + size_, attributeName_, tex_name, true, attrType.isColor, attrType.isCamera, 
			attrType.isNoInterpolate, attrType.isAnimation, attrType.isTruncation, attrType. isQuaternion);

		insertItem(type, typeName, type_, num);
		_reInitFrameToIdx();
	}

	void TimelineComponent::dealWithRootMotionIdx(int st,int size)
	{
		int deleteIdx = -1;
		for (int i = 0 ; i < mRootMotionData.idx.size() ; ++ i)
		{
			if (mRootMotionData.idx[i].first == st)
			{
				deleteIdx = i;
				break;
			}
			if (mRootMotionData.idx[i].first > st)
			{
				mRootMotionData.idx[i].first -= size;
				mRootMotionData.idx[i].second -= size;
			}
		}
		if (deleteIdx != -1)
		{
			mRootMotionData.idx.erase(mRootMotionData.idx.begin() + deleteIdx);
			if (deleteIdx < mRootMotionData.type.size())
				mRootMotionData.type.erase(mRootMotionData.type.begin() + deleteIdx);
		}
	}
	void TimelineComponent::dealWithRootMotionIdxAdd(int en)
	{
		for (int i = 0; i < mRootMotionData.idx.size(); ++i)
		{
			if (mRootMotionData.idx[i].first >= en)
			{
				mRootMotionData.idx[i].first ++;
				mRootMotionData.idx[i].second ++;
			}
		}
	}
	void TimelineComponent::addRootMotionIdx(const std::string& combineName, const std::string& attrName, int size)
	{
		if (combineName == mRootMotionData.comName)
		{
			mRootMotionData.idx.emplace_back(std::make_pair(OrderCnt, OrderCnt + size));

			if (attrName == positionName_.c_str())
				mRootMotionData.type.emplace_back(RootMotionDataType::M_Position);
			else if (attrName == rotationName_.c_str())
				mRootMotionData.type.emplace_back(RootMotionDataType::M_Rotation);
			else if (attrName == scaleName_.c_str())
				mRootMotionData.type.emplace_back(RootMotionDataType::M_Scale);
		}
	}
#endif

	

	void TimelineComponent::addActorAndComMaps(const std::string& actName, ActorPtr act, const std::string& comName, EchoWeakPtr<ActorComponent> com)
	{
		mActorMaps.insert(std::make_pair(actName, act));
		mComMaps.insert(std::make_pair(comName, com));

		if (!com.expired())
			com->setTimelineCom(this);

		//属性过滤器相关
#ifdef ECHO_EDITOR
		addTypeMapItem(comName, com);
#endif
	}

	void TimelineComponent::revokeRemovalActAndComMaps(const std::string& actName, ActorPtr act, const std::string& comName, EchoWeakPtr<ActorComponent> com)
	{
		mActorMaps[actName] = act;
		mComMaps[comName] = com;

		if (!com.expired())
			com->setTimelineCom(this);

		//属性过滤器相关
#ifdef ECHO_EDITOR
		addTypeMapItem(comName, com);
#endif
	}

	void TimelineComponent::importItem(const EchoHermite::HermiteType& type, const Name& typeName, const FragmentType& type_)
	{
		size_t temp;
		if (mHermites.size() < OrderCnt)
		{
			temp = OrderCnt - mHermites.size();
			for (int i = 0; i < temp; i++)
			{
				AddProperty(type, typeName,type_);
			}
		}
	}

	void TimelineComponent::insertItem(const EchoHermite::HermiteType& type, const Name& typeName, const FragmentType& type_, int num)
	{
		size_t temp = 0;

		if (mHermites.size() < OrderCnt)
		{
			temp = OrderCnt - mHermites.size();
			mHermites.resize(OrderCnt);
#ifdef ECHO_EDITOR
			showCurveVec.resize(OrderCnt);
			HermiteNames.resize(OrderCnt);
#endif
			for (size_t i = mHermites.size() - 1; i >= num + temp; --i)
			{
				mHermites[i] = mHermites[i - temp];
#ifdef ECHO_EDITOR
				showCurveVec[i] = showCurveVec[i - temp];
				HermiteNames[i] = HermiteNames[i - temp];
#endif
			}
		}

		for (int i = 0; i < temp; i++)
		{
			EchoHermite* hermite = nullptr;
			if (typeName == Name::g_cEmptyName)
			{
				if (type == EchoHermite::Float_Hermite)
					hermite = new EchoFloatHermite();
				else if (type == EchoHermite::Bool_Hermite)
					hermite = new EchoBoolHermite();
				else if (type == EchoHermite::Fragment_Hermite)
					hermite = new EchoFragmentHermite(type_);
				else if (type == EchoHermite::String_Hermite)
					hermite = new EchoStringHermite();
				else
					hermite = new EchoFloatHermite();
			}
			else
			{
				if (EchoHermiteFactortManager::getSingleton().getHermiteFactory(typeName) == nullptr)
					assert(!"this hermitekey is not register");
				hermite = EchoHermiteFactortManager::getSingleton().getHermiteFactory(typeName)->createHermite();
			}

			mHermites[i + num] = hermite;

#ifdef ECHO_EDITOR
			showCurveVec[i + num] = 1;
			HermiteNames[i + num] = "";
#endif
		}
	}

	void TimelineComponent::dealwithCameraComEndPlay()
	{
		if (mCameraActiveIdx != -1)
		{
			CameraBaseComponent* activeCamera = CameraComponentManager::instance()->getActiveCameraCom();
			if (activeCamera)
				activeCamera->setIsUse(false);
			mCameraActiveIdx = -1;
		}
	}
#ifdef ECHO_EDITOR
	void TimelineComponent::setComTypeMappingState(const std::string typeName,  bool isShow)
	{
		for (const auto& actorIter : mActorToComponents)
		{
			for (const std::string& comIter : actorIter.second)
			{
				if (mComTypeMappingMaps[comIter] == typeName)
				{
	                if (isShow)
					{
						if(mActorShows[actorIter.first] > 0)
						    mActorShows[actorIter.first]--;
					}
					else
					{
						mActorShows[actorIter.first]++;
					}
				}
			}
		}
	}
#endif
	void TimelineComponent::getTopRootNode()
	{
		std::pair<int, MotionNodeData>  topRootNodeData;
		topRootNodeData.first = INT_MAX;
		for (const auto& actIter : mActorToComponents)
		{
			for (const auto& comIter : actIter.second)
			{
				const std::string& comName_ = comIter.c_str();
				if (comName_.find("rootNode") == std::string::npos)
					continue;
				if (mActorMaps.find(actIter.first) == mActorMaps.end())
					continue;

				int level = -1;
				ActorPtr tempActor = mActorMaps[actIter.first];
				while(tempActor != nullptr)
				{
					tempActor = tempActor->getParentActor();
					++level;
				}
				if (level >= topRootNodeData.first)
					break;
				bool isExit = false;
				for (const auto& attrIter : mComponentToAttributes[comIter])
				{
					if (attrIter.attributeName == "Position")
					{
						topRootNodeData.second.idx.push_back(std::make_pair(attrIter.st,attrIter.en));
						topRootNodeData.second.type.push_back(M_Position);
						isExit = true;
					}
					else if (attrIter.attributeName == "Rotation")
					{
						topRootNodeData.second.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
						topRootNodeData.second.type.push_back(M_Rotation);
						isExit = true;
					}
					else if (attrIter.attributeName == "Scale")
					{
						topRootNodeData.second.idx.push_back(std::make_pair(attrIter.st, attrIter.en));
						topRootNodeData.second.type.push_back(M_Scale);
						isExit = true;
					}
				}
				if (isExit)
				{
				   topRootNodeData.first = level;
                   topRootNodeData.second.comName = comIter.c_str();
				   break;
				}
				
			}
		}
		mRootMotionData = topRootNodeData.second;
	}

	void TimelineComponent::stopTodTimelinePlay()
	{
		ProtocolComponentSystem::instance()->callProtocolFunc(Echo::Name("StopTodTimelinePlay"), nullptr, 0);
	}

	void TimelineComponent::setOneSecondFrame(int frame_)
	{
		mDeltaTime = static_cast<float>(frame_);
	}


#ifdef ECHO_EDITOR
	void TimelineComponent::addKeySetData(int idx_, int frame_)
	{
		if (mIsControlKeyByKeySet)
			return;
		std::string attrName_;
		if (mIdxToAttr.count(idx_) == 0)
		{
			std::map<int, std::pair<std::string,int>>::iterator iter = mIdxToAttr.lower_bound(idx_);
			if (iter != mIdxToAttr.begin())
			{
				--iter;
				attrName_ = iter->second.first;
			}
		}
		else
			attrName_ = mIdxToAttr[idx_].first;

		if (attrName_.empty())
			return;
		std::map<int, std::set<int>>&keySetMap_ = mAttrToFrameSet[attrName_];
		keySetMap_[frame_].insert(idx_);

		mFrameToAttr[frame_].insert(attrName_);
	}

	void TimelineComponent::deleteKeySetData(int idx_, int frame_)
	{
		if (mIsControlKeyByKeySet)
			return;
		std::string attrName_;
		if (mIdxToAttr.count(idx_) == 0)
		{
			std::map<int, std::pair<std::string, int>>::iterator iter = mIdxToAttr.lower_bound(idx_);
			if (iter != mIdxToAttr.begin())
			{
				--iter;
				attrName_ = iter->second.first;
			}
		}
		else
			attrName_ = mIdxToAttr[idx_].first;

		if (attrName_.empty())
			return;

		if (mAttrToFrameSet.count(attrName_) == 0)
			return;

		std::map<int, std::set<int>>& keySetMap_ = mAttrToFrameSet[attrName_];
		if (keySetMap_.count(frame_) == 0)
			return;

		if (keySetMap_[frame_].count(idx_) != 0)
		{
			keySetMap_[frame_].erase(idx_);
			if (keySetMap_[frame_].empty())
			{
                 keySetMap_.erase(frame_);
				 mFrameToAttr[frame_].erase(attrName_);
				 if (mFrameToAttr[frame_].empty())
					 mFrameToAttr.erase(frame_);
			}
				
			if (mAttrToFrameSet[attrName_].empty())
			{
				mAttrToFrameSet.erase(attrName_);
			}
		}
	}

	void TimelineComponent::initKeySetData()
	{
		for (int i = 0; i < mHermites.size(); ++i)
		{
			if (mHermites[i] == nullptr)
				return;
			if (i == mCameraMgrIdx)
				continue;
			for (const auto& iter : mHermites[i]->mKeyMap)
			{
				addKeySetData(i,iter.first);
			}
		}
	}

	void TimelineComponent::resetKeySetData()
	{
		mAttrToFrameSet.clear();
		mFrameToAttr.clear();
	}

	void TimelineComponent::initIdxToAttr()
	{
		for (const auto& comIter : mComponentToAttributes)
		{
			for (const auto& attrIter : comIter.second)
			{
				mIdxToAttr[attrIter.st] = std::make_pair(comIter.first + " : " + attrIter.attributeName.c_str(), attrIter.en);
			}
		}
	}

	void TimelineComponent::resetIdxToAttr()
	{
		mIdxToAttr.clear();
	}

	void TimelineComponent::moveKeySetData(int idx_, int oriFrame_, int resultFrame_)
	{
		deleteKeySetData(idx_, oriFrame_);
		addKeySetData(idx_, resultFrame_);
	}

	void TimelineComponent::moveKeySetData(const std::vector<int>& idxVec_, const std::vector<int>& oriFrame_, int diffFrame_)
	{
		if (idxVec_.size() != oriFrame_.size())
			return;

		for (int i = 0; i < idxVec_.size(); ++i)
		{
			deleteKeySetData(idxVec_[i], oriFrame_[i]);
		}

		std::vector<int> resultFrame_;
		resultFrame_.resize(idxVec_.size());
		for (int i = 0; i < idxVec_.size(); ++i)
		{
			resultFrame_[i] =  std::max(0, oriFrame_[i] + diffFrame_);
		}
		for (int i = 0; i < idxVec_.size(); ++i)
		{
			addKeySetData(idxVec_[i], resultFrame_[i]);
		}
	}

	void TimelineComponent::moveKeyByAllKeySet(int oldFrame_, int newFrame_)
	{
		newFrame_ = std::max(0, newFrame_);
		if (oldFrame_ == newFrame_)
			return;
		if (mFrameToAttr.count(oldFrame_) == 0)
			return;
		std::set<std::string>  value_ = mFrameToAttr[oldFrame_];
		for (const std::string& iter : value_)
		{
			moveKeyByGroupKeySet(oldFrame_, newFrame_,iter);
		}
		mFrameToAttr.erase(oldFrame_);
		

	}

	void TimelineComponent::deleteKeyByGroupKeySet(int frame_, const std::string& attrName_)
	{
		if (mAttrToFrameSet.count(attrName_) == 0)
			return;

		mIsControlKeyByKeySet = true;
		  
		if (mAttrToFrameSet[attrName_].count(frame_) != 0)
		{
			for (int idxIter : mAttrToFrameSet[attrName_][frame_])
			{
				DeleteKey(idxIter,frame_);
			}

			mAttrToFrameSet[attrName_].erase(frame_);
			mFrameToAttr[frame_].erase(attrName_);
			if(mFrameToAttr[frame_].empty())
				mFrameToAttr.erase(frame_);
		}

		mIsControlKeyByKeySet = false;
	}

	void TimelineComponent::addKeyByGroupKeySet(int frame_, const std::string& attrName_)
	{
		int st = -1;
		int en = -1;
		for (const auto& iter : mIdxToAttr)
		{
			if(iter.second.first == attrName_)
			{
				st = iter.first;
				en = iter.second.second;
				break;
			}
		}
		if (st == -1 || en == -1)
			return;

		mIsControlKeyByKeySet = true;

		for (int i = st; i < en; ++i)
		{
			if (mAttrToFrameSet[attrName_][frame_].count(i) == 0)
			{
				mAttrToFrameSet[attrName_][frame_].insert(i);
				if (mHermites.size() <= i)
					continue;
				if (mHermites[i]->mHermiteType == EchoHermite::Float_Hermite)
					AddKey(i, frame_, floatInterpolation(i, (float)(frame_)));
				else if (mHermites[i]->mHermiteType == EchoHermite::Bool_Hermite)
				{
					bool isCamera = false;
					for (const auto& iter : mCameraBaseComMap)
					{
						if (iter.second == i)
						{
							isCamera = true;
							break;
						}
					}
					if(!isCamera)
					   AddKey(i, frame_, boolInterpolation(i, float(frame_)));
					else
						AddKey(i, frame_, true);
				}
				else if (mHermites[i]->mHermiteType == EchoHermite::Fragment_Hermite)
					AddKey(i, frame_, 1);
				else if (mHermites[i]->mHermiteType == EchoHermite::String_Hermite)
				{
					std::string keyValue_;
					stringInterpolation(i, (float)frame_, keyValue_);
					AddKey(i, frame_, keyValue_);
				}
			}
		}

		if (mFrameToAttr[frame_].count(attrName_) == 0)
			mFrameToAttr[frame_].insert(attrName_);

		mIsControlKeyByKeySet = false;
	}

	void TimelineComponent::moveKeyByGroupKeySet(int oldFrame_, int newFrame_, const std::string& attrName_)
	{
		newFrame_ = std::max(0, newFrame_);
		if (mAttrToFrameSet.count(attrName_) == 0)
			return;
		if (mAttrToFrameSet[attrName_].count(oldFrame_) == 0)
			return;

		mIsControlKeyByKeySet = true;

		std::vector<int> idxVec_;
		for (int idxIter : mAttrToFrameSet[attrName_][oldFrame_])
		{
			idxVec_.emplace_back(idxIter);
		}

	
		std::vector<int> frameVec_;
		frameVec_.resize(idxVec_.size(), oldFrame_);
		int diffFrame_ = newFrame_ - oldFrame_;

		for (int iter :  mAttrToFrameSet[attrName_][oldFrame_])
		{
			mAttrToFrameSet[attrName_][newFrame_].insert(iter);
			mFrameToAttr[newFrame_].insert(attrName_);
		}


		mAttrToFrameSet[attrName_].erase(oldFrame_);

		if (mFrameToAttr.count(oldFrame_) != 0)
		{
			mFrameToAttr[oldFrame_].erase(attrName_);
			if (mFrameToAttr[oldFrame_].empty())
				mFrameToAttr.erase(oldFrame_);
		}

		if (!idxVec_.empty())
		{
			if (idxVec_.size() > 1)
				moveGroupKey(idxVec_, frameVec_, diffFrame_, 0.f);
			else
				MoveKey(idxVec_[0], frameVec_[0], frameVec_[0] + diffFrame_);
		}

		mIsControlKeyByKeySet = false;
	}

	void TimelineComponent::deleteKeyByAllKeySet(int frame_)
	{
		std::set<std::string>  value_  = mFrameToAttr[frame_];
		for (const std::string& iter : value_)
		{
			deleteKeyByGroupKeySet(frame_,iter);
		}
		mFrameToAttr.erase(frame_);
	}

	void TimelineComponent::addKeyByAllKeySet(int frame_)
	{
		for (const auto& iter : mIdxToAttr)
		{
			addKeyByGroupKeySet(frame_, iter.second.first);
		}
	}


#endif

	void TimelineComponent::setTimelineFrame(int frame_)
	{
		currTime = static_cast<float>(frame_) / mDeltaTime;
	}

	bool TimelineComponent::refreshExitAttributeActor()
	{
		bool isExitMissing = false;
		for (auto& actPair : mActorMaps)
		{
			if (actPair.second.expired() || actPair.second == nullptr)
			{
				const std::map<int, ActorPtr>& idToActor_ = mActorIdData.mActorIdToActor;
				std::map<std::string, int>::const_iterator idIter = mActorToId.find(actPair.first);
				if (idIter != mActorToId.end() && idIter->second != -1 && idToActor_.contains(idIter->second))
				{
					actPair.second = idToActor_.at(idIter->second);
				}
				else
				{
					PickActorResult result = getActor()->getActorComByRelativeName(actPair.first, false);
					if(!result.seleteActor.expired())
					   actPair.second = result.seleteActor;
				}

				if (!actPair.second.expired())
					initComData(actPair.first,actPair.second);
				else
					isExitMissing = true;
			}
		}
		return isExitMissing;
	}

	bool TimelineComponent::refreshExitAttributeCom()
	{
		bool isExitMissing = false;
		for (auto& comPair : mComMaps)
		{
			if (comPair.second.expired() || comPair.second == nullptr)
			{
				String realName = comPair.first;
				auto i = realName.find(':');
				realName.replace(i - 1, 3, "&&");
				PickActorResult result = getActor()->getActorComByRelativeName(realName, false);
				comPair.second = result.seleteCom;

				if (!result.seleteCom.expired())
					result.seleteCom->setTimelineCom(this);
				else
					isExitMissing = true;
			}
		}
		return isExitMissing;
	}

	bool TimelineComponent::judgeIsExitMissingAttribute()
	{
		for (const auto& iter : mActorMaps)
		{
			if (iter.second.expired())
				return false;
		}
		for(const auto& iter : mComMaps)
		{
			if (iter.second.expired())
				return false;
		}
		return true;
	}


#ifdef ECHO_EDITOR
	void TimelineComponent::setShowTimeMinByFrame(int frame)
	{
		mShowTimeMin = static_cast<float>(frame) / mDeltaTime;
	}

	void TimelineComponent::setShowTimeMaxByFrame(int frame)
	{
		mShowTimeMax = static_cast<float>(frame) / mDeltaTime;
	}

	bool TimelineComponent::recoredRootMotionData(const attributeData& attrData_, std::vector<float>& data)
	{
		if (!mIsUseRootMotionSkew)
			return false;
		bool isRootMotion = judgeIsRootMotionNode(attrData_);
		if (!isRootMotion)
			return false;
		if (attrData_.attributeName == positionName_)
		{
			mRootMotionPosition.first = true;
			mRootMotionPosition.second.x = data[0];
			mRootMotionPosition.second.y = data[1];
			mRootMotionPosition.second.z = data[2];
		}
		else if (attrData_.attributeName == rotationName_)
		{
			mRootMotionRotation.first = true;
			mRootMotionRotation.second.x = data[0];
			mRootMotionRotation.second.y = data[1];
			mRootMotionRotation.second.z = data[2];
		}
		else if (attrData_.attributeName == scaleName_)
		{
			mRootMotionScale.first = true;
			mRootMotionScale.second.x = data[0];
			mRootMotionScale.second.y = data[1];
			mRootMotionScale.second.z = data[2];
		}
		return true;
	}

	bool TimelineComponent::judgeIsRootMotionNode(const attributeData& attrData_)
	{
		for (const auto& motionIdx : mRootMotionData.idx)
		{
			if(attrData_.st == motionIdx.first && attrData_.en == motionIdx.second)
				return true;
		}
		return false;
	}

	void TimelineComponent::decreaseRootMotionNodeSkew(const attributeData& attrData_, std::vector<float>& data,ActorComponent* currCom)
	{

		if (!mIsUseRootMotionSkew || currCom == nullptr)
			return;

		bool isRootMotion = judgeIsRootMotionNode(attrData_);
		if (!isRootMotion)
			return;

		Vector3 pos_ = currCom->getPosition();
		Vector3 rot_ = currCom->getEulerAngles();
		Vector3 scale_ = currCom->getScale();

		if (attrData_.attributeName == positionName_)
			pos_ = Vector3(data[0],data[1],data[2]);
		else if(attrData_.attributeName == rotationName_)
			rot_ = Vector3(data[0], data[1], data[2]);
		else if(attrData_.attributeName == scaleName_)
			scale_ = Vector3(data[0], data[1], data[2]);

		Quaternion parentOrientation;
		parentOrientation.FromEuler(mSkewRotation);

		Quaternion localOrientation;
		localOrientation.FromEuler(rot_);

		localOrientation = localOrientation * parentOrientation.Inverse();

	    rot_  = localOrientation.ToEuler();

		scale_ = scale_ / mSkewScale;
		pos_ -= mSkewPosition;
		pos_ = (parentOrientation.Inverse() * pos_) / mSkewScale;
        
		if (attrData_.attributeName == positionName_)
		{
			data[0] = pos_[0];
			data[1] = pos_[1];
			data[2] = pos_[2];
		}
		else if (attrData_.attributeName == rotationName_)
		{
			data[0] = rot_[0];
			data[1] = rot_[1];
			data[2] = rot_[2];
		}
		else if (attrData_.attributeName == scaleName_)
		{
			data[0] = scale_[0];
			data[1] = scale_[1];
			data[2] = scale_[2];
		}
	}

	void TimelineComponent::setSkewPosition(const Vector3& skewPos_)
	{
		mSkewPosition = skewPos_;
	}

	void TimelineComponent::setSkewRotation(const Vector3& skewRot_)
	{
		mSkewRotation = skewRot_;
	}

	void TimelineComponent::setSkewScale(const Vector3& skewScale_)
	{
		mSkewScale = skewScale_;
	}

	void TimelineComponent::increaseRootMotionNodeSkew(ActorComponent* currCom)
	{
		if (!mIsUseRootMotionSkew || currCom == nullptr)
			return;

		if (!mRootMotionPosition.first)
			mRootMotionPosition.second = Vector3(0.f, 0.f, 0.f);

		if (!mRootMotionRotation.first)
			mRootMotionRotation.second = Vector3 (0.f,0.f,0.f);

		if (!mRootMotionScale.first)
			mRootMotionScale.second = Vector3(1.f, 1.f, 1.f);
		

		Quaternion parentOrientation;
		parentOrientation.FromEuler(mSkewRotation);
				
		Quaternion localOrientation;
		localOrientation.FromEuler(mRootMotionRotation.second);

		localOrientation = parentOrientation * localOrientation;

		mRootMotionRotation.second = localOrientation.ToEuler();
	
		mRootMotionScale.second = mSkewScale * mRootMotionScale.second;

		mRootMotionPosition.second = parentOrientation * (mSkewScale * mRootMotionPosition.second);

		mRootMotionPosition.second += mSkewPosition;

		NodeActorComponent* com = static_cast<NodeActorComponent*>(currCom);
		com->setPosition(mRootMotionPosition.second);
		com->setEulerAngles(mRootMotionRotation.second);
		com->setScale(mRootMotionScale.second);

		ProtocolComponentSystem::instance()->callProtocolFunc(Echo::PFID_UpdateAxisTransform, nullptr, 0);

		mRootMotionPosition.first = false;
		mRootMotionRotation.first = false;
		mRootMotionScale.first = false;

		/*std::string logInfo = "";
		logInfo.append("Pos : ").append(std::to_string(mRootMotionPosition.second.x)).append(" ").append(std::to_string(mRootMotionPosition.second.y)).append(" ").append(std::to_string(mRootMotionPosition.second.z)).append(" \n");
		logInfo.append("Rot : ").append(std::to_string(mRootMotionRotation.second.x)).append(" ").append(std::to_string(mRootMotionRotation.second.y)).append(" ").append(std::to_string(mRootMotionRotation.second.z)).append(" \n");
		logInfo.append("Scale : ").append(std::to_string(mRootMotionScale.second.x)).append(" ").append(std::to_string(mRootMotionScale.second.y)).append(" ").append(std::to_string(mRootMotionScale.second.z)).append(" \n");
		LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "transForm Data : " + logInfo);*/
	}

	void TimelineComponent::setExitAttributeQuaternion(int id, bool isQuaternion)
	{
		for (auto& comToAttr : mComponentToAttributes/* | std::views::values*/)
		{
			std::vector<attributeData>& attrData = comToAttr.second;
			auto iter = std::find_if(attrData.begin(), attrData.end(), [id](attributeData& a) {return id >= a.st && id < a.en; });
			if (iter == attrData.end()) continue;

			iter->isQuaternion = isQuaternion;
			break;
		}
	}

	void TimelineComponent::setExitAttributeQuaternion(const std::string& combineName, const std::string& attributeName, bool isQuaternion)
	{
		if (mComponentToAttributes.find(combineName) == mComponentToAttributes.end())
			return ;
		const auto& attributeDatas = mComponentToAttributes[combineName];
		auto iter = std::find_if(mComponentToAttributes[combineName].begin(), mComponentToAttributes[combineName].end(),
			[&attributeName](const attributeData& lhs) {
				return lhs.attributeName == Name(attributeName);
			});
		if (iter != attributeDatas.end())
			iter->isQuaternion = isQuaternion;
	}

	bool TimelineComponent::isExitAttribute(const std::string& combineName, const std::string& attributeName)
	{
		if (mComponentToAttributes.find(combineName) == mComponentToAttributes.end())
			return false;
		const auto& attributeDatas = mComponentToAttributes[combineName];
		auto iter = std::find_if(mComponentToAttributes[combineName].begin(), mComponentToAttributes[combineName].end(),
			[&attributeName](const attributeData& lhs) {
				return lhs.attributeName == Name(attributeName);
			});
		if (iter != attributeDatas.end())
			return true;
		return false;
	}

	bool TimelineComponent::isExitAttributeActor(const std::string& actName_)
	{
		if(mActorMaps.find(actName_) == mActorMaps.end())
		    return false;
		if (mActorMaps[actName_].expired() || mActorMaps[actName_] == nullptr)
			return false;
		return true;
	}

	bool TimelineComponent::isExitAttributeCom(const std::string& iterName_,const std::string& comName_)
	{
		if (mComMaps.find(iterName_) == mComMaps.end())
			return false;
		auto currCom = mComMaps[iterName_];
		if (currCom.expired() || currCom == nullptr)
			return false;
		return true;
	}


	void TimelineComponent::getAttributeData(int pHermiteIndex, attributeData& attrData_)
	{
		for(const auto& com: mComponentToAttributes)
			for (const auto& iter : com.second)
			{
				if (iter.st <= pHermiteIndex && iter.en >= pHermiteIndex)
				{
					attrData_ = iter;
					return;
				}
			}

		assert(!"Error not find attributeData");
	}

	void TimelineComponent::getAttributeData(const std::string& combineName, const std::string& attrName,attributeData& attrData_)
	{
		if (mComponentToAttributes.find(combineName) == mComponentToAttributes.end())
			return;
		for (const auto& iter : mComponentToAttributes[combineName])
		{
			if (iter.attributeName == Name(attrName))
			{
				attrData_ = iter;
				break;
			}
		}
	}

	void TimelineComponent::getAllActorData(ActorPtr currActor,bool isFindNoTimelineActor)
	{
		const EchoTMap<String, ActorPtr >& childActors = currActor->getChildActors();
		for (const auto& iter : childActors)
		{
			if (iter.second.expired())
				continue;
			if (!isFindNoTimelineActor  && iter.second->getComponentsByType<TimelineComponent>().empty())
			{
				getAllActorData(iter.second, isFindNoTimelineActor);
				continue;
			}
			
			String  absName = iter.second->getCombinedID();
			std::string  relNameStr = getActor()->getRelativeNameFromAbsoluteName(absName);
			std::string absNameStr = absName;
			deleteScenePrefix(absNameStr);
			mRelNameToAbsName[relNameStr] = absNameStr;
			mAllActorsMaps[iter.second] = relNameStr;
			getAllActorData(iter.second, isFindNoTimelineActor);
		}
	}

	void TimelineComponent::getCurrAtrorData(ActorPtr currActor)
	{
		String  absName = currActor->getCombinedID();
		std::string  relNameStr = getActor()->getRelativeNameFromAbsoluteName(absName);
		std::string absNameStr = absName;
		mRelNameToAbsName[relNameStr] = absNameStr;
		mAllActorsMaps[currActor] = relNameStr;
	}
	void TimelineComponent::clearEditorActorData()
	{
		mAllActorsMaps.clear();
	}
	void TimelineComponent::setHermiteNames(int i, const std::string& name)
	{
		if (i + 1 > HermiteNames.size())
			return;
		HermiteNames[i] = name;
	}

	std::string TimelineComponent::getHermiteNames(size_t i)
	{
		if (i + 1 > HermiteNames.size())
		{
			HermiteNames.resize(i + 1);
			return "";
		}
		return HermiteNames[i];
	}
#endif

	int TimelineComponent::getTimeLineFrame()
	{
		return static_cast<int>(currTime*mDeltaTime + 0.5);
	}

	float TimelineComponent::getRealTime()
	{
		return currTime * mDeltaTime;
	}

	void TimelineComponent::setTimelineStartTime(float startTime)
	{
		startSceneTime = startTime;
	}

	void TimelineComponent::setTimelineEndTime(float endTime_)
	{
		endSceneTime = endTime_;
	}

	float TimelineComponent::getTimelineStartTime()
	{
		return startSceneTime;
	}

	float TimelineComponent::getTimelineEndTime()
	{
		return endSceneTime;
	}

	float TimelineComponent::getTimelineTimeLength()
	{
		return endSceneTime - startSceneTime;
	}

	void EchoTimelineProFuncImp::initProtocolFunction()
	{
	}

	void EchoTimelineProFuncImp::uninitProtocolFunction()
	{

	}

	EchoTimelineScript::EchoTimelineScript(const EchoActorScript& pActor, const std::string& name)
		:EchoActorComponentScript(pActor, name)
	{
	}

	EchoTimelineScript  EchoTimelineScript::GetTimeLineCom(const EchoActorScript& pActor, const std::string & name)
	{
		return EchoTimelineScript(pActor, name);
	}

	TimelineComponentInfo::TimelineComponentInfo(const Name& name,ActorCOMMoveableType moveable):
		ActorComponentInfo(name,moveable)
	{
	}

	TimelineComponentInfo::~TimelineComponentInfo()
	{
		for (auto& hermite : mHermites)
		{
			for (auto& hKey : hermite->mKeyMap)
			{
				SAFE_DELETE(hKey.second);
			}
			SAFE_DELETE(hermite);
		}
		mHermites.clear();
	}

	void TimelineComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<TimelineComponentInfo>::get()->ExportJson(this, jsonNode, "");

		
	}


	void TimelineComponentInfo::ImportData(cJSON * jsonNode)
	{   
		reflect::TypeResolver<TimelineComponentInfo>::get()->ImportJson(this, jsonNode, "");

		for (auto& hermite : mHermites)
		{
			for (auto& hKey : hermite->mKeyMap)
			{
				SAFE_DELETE(hKey.second);
			}
			SAFE_DELETE(hermite);
		}
		mHermites.clear();

		cJSON* pHermiteArray = cJSON_GetObjectItem(jsonNode, "HermiteArray");
		if (pHermiteArray)
		{
			isLoadHermite = true;
			for (cJSON* pChild = pHermiteArray->child; nullptr != pChild; pChild = pChild->next)
			{
				EchoHermite::HermiteType hermiteType_ = EchoHermite::Float_Hermite;
				EchoHermite* hermite = nullptr;
				//int flag = false;
				for (cJSON* pKey = pChild->child; pKey != nullptr; pKey = pKey->next)
				{
					int weight = 0;
					if (!ReadJSNode(pKey, "weight", weight))
					{
						hermiteType_ = static_cast<EchoHermite::HermiteType>(pKey->valueint);
						if (hermiteType_ == EchoHermite::Float_Hermite)
						{
							hermite = new EchoFloatHermite();
							hermite->mHermiteType = EchoHermite::Float_Hermite;
						}
						else if (hermiteType_ == EchoHermite::Bool_Hermite)
						{
							hermite = new EchoBoolHermite();
							hermite->mHermiteType = EchoHermite::Bool_Hermite;
						}
						else if (hermiteType_ == EchoHermite::Fragment_Hermite)
						{
							int fragmentType_ = FragmentType::Default_;
							pKey = pKey->next;
							fragmentType_ = pKey->valueint;
							hermite = new EchoFragmentHermite(static_cast<FragmentType>(fragmentType_));
							hermite->mHermiteType = EchoHermite::Fragment_Hermite;
						}
						continue;
					}
					if (hermite == nullptr)
						hermite = new EchoFloatHermite();
					auto value = cJSON_GetObjectItem(pKey, "value");
					if (value)
					{
						HermiteFloatKey* hfKey = new HermiteFloatKey();
						ReadJSNode(pKey, "value", hfKey->value);
						ReadJSNode(pKey, "leftHandTangent", hfKey->leftHandTangent);
						ReadJSNode(pKey, "rightHandTangent", hfKey->rightHandTangent);
						ReadJSNode(pKey, "interpolateMode", hfKey->interpolateMode);
						ReadJSNode(pKey, "leftMode", hfKey->leftMode);
						ReadJSNode(pKey, "rightMode", hfKey->rightMode);
						hermite->mKeyMap.emplace(weight, hfKey);
					}

					auto boolValue = cJSON_GetObjectItem(pKey, "boolvalue");
					if (boolValue)
					{
						HermiteBoolKey* hbKey = new HermiteBoolKey();
						ReadJSNode(pKey, "boolvalue", hbKey->value);
						ReadJSNode(pKey, "leftHandTangent", hbKey->leftHandTangent);
						ReadJSNode(pKey, "rightHandTangent", hbKey->rightHandTangent);
						ReadJSNode(pKey, "interpolateMode", hbKey->interpolateMode);
						hermite->mKeyMap.emplace(weight, hbKey);
					}

					auto fragmentValue = cJSON_GetObjectItem(pKey, "fragmentvalue");
					if (fragmentValue)
					{
						HermiteFragmentKey* fragmentKey = new HermiteFragmentKey();
						ReadJSNode(fragmentValue, "startTime", fragmentKey->value.startTime);
						ReadJSNode(fragmentValue, "length", fragmentKey->value.length);
						ReadJSNode(pKey, "leftHandTangent", fragmentKey->leftHandTangent);
						ReadJSNode(pKey, "rightHandTangent", fragmentKey->rightHandTangent);
						ReadJSNode(pKey, "interpolateMode", fragmentKey->interpolateMode);
						hermite->mKeyMap.emplace(weight, fragmentKey);
					}
				}
				mHermites.emplace_back(hermite);
				mMaxWeight = std::max(mMaxWeight, hermite->GetMaxLength());
				hermite = nullptr;
			}
		}

		cJSON* deltaTime = cJSON_GetObjectItem(jsonNode, "mDeltaTime");
		if (deltaTime)
		{
			isLoadDeltaTime = true;
			mDeltaTime = static_cast<float>(deltaTime->valuedouble);
		}

		cJSON* startTime = cJSON_GetObjectItem(jsonNode, "startTime");
		if (startTime)
		{
			isLoadStartTime = true;
			startSceneTime = static_cast<float>(startTime->valuedouble);
		}
		cJSON* endTime = cJSON_GetObjectItem(jsonNode, "endTime");
		if (endTime)
		{
			isLoadEndTime = true;
			endSceneTime = static_cast<float>(endTime->valuedouble);
		}
		cJSON* currentPlayWay_ = cJSON_GetObjectItem(jsonNode, "currentPlayWay");
		if (currentPlayWay_)
		{
			isLoadPlayWay = true;
			currentPlayWay = currentPlayWay_->valueint;
		}
	}

	void TimelineComponentInfo::CopyTo(ActorComponentInfo * info)
	{
		if (info == nullptr)
			return;
		TimelineComponentInfo* newObjectInfo = static_cast<TimelineComponentInfo*>(info);
		if (newObjectInfo == nullptr)
			return;
		reflect::TypeResolver<TimelineComponentInfo>::get()->copyTo(this,newObjectInfo);
		//newObjectInfo->m_TimelineFilePath = m_TimelineFilePath;
	}

	void TimelineComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void TimelineComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<TimelineComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
#ifdef ECHO_EDITOR
	bool TimelineComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		//等客户端处理完时间轴资源收集之后，需要把后面的那个判断去掉
		if(m_TimelineFilePath.empty() || m_TimelineFilePath.find(".fclip") == std::string::npos)
		   return false;
		resMap.insert(std::make_pair(ResourceType::NeedCopyRes,m_TimelineFilePath));
		_parseTimelineFile(resMap,m_TimelineFilePath);
		return true;
	}
	void TimelineComponentInfo::_parseTimelineFile(std::unordered_multimap<ResourceType, std::string>& resMap, const std::string& filePath)
	{
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(filePath.data()));
		if (pDataStream.isNull())
			return;
		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->read(pData, nSize);
		cJSON* pRoot = cJSON_Parse(pData);
		delete[] pData;
		if (pRoot == nullptr)
			return;

		cJSON* pHermiteArray = cJSON_GetObjectItem(pRoot, "HermiteArray");
		if (pHermiteArray)
		{
			for (cJSON* pChild = pHermiteArray->child; nullptr != pChild; pChild = pChild->next)
			{
				for (cJSON* pKey = pChild->child; pKey != nullptr; pKey = pKey->next)
				{
					std::string texName_ = "";
					if (ReadJSNode(pKey, "stringvalue", texName_))
					{
						if(!texName_.empty())
						   resMap.insert(std::make_pair(ResourceType::NeedCopyRes, texName_));
					}
				}
			}
		}
		cJSON_Delete(pRoot);
	}
#endif
	float EchoTimelineScript::getActorTimeValue(const size_t& vIdx)
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			return pCom->getActorTimeValue(vIdx);
		return 0.f;
	}

	void EchoTimelineScript::remakeTime()
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			pCom->remakeTime();
	}


	void EchoTimelineScript::setIsPlay(bool flag)
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			pCom->setIsPlay(flag);
	}

	bool EchoTimelineScript::getIsPlayScene()
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			return pCom->getIsPlayScene();
		return false;
	}
	int  EchoTimelineScript::getTimeLineFrame()
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			return pCom->getTimeLineFrame();
		return -1;
	}

	void EchoTimelineScript::GetTimeLineFile(const char* path, bool synchronous, bool isChangeState)
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			pCom->GetTimeLineFile(path,synchronous,isChangeState);
	}

	void EchoTimelineScript::stopTodTimelinePlay()
	{
		auto pCom = static_cast<TimelineComponent*>(m_Ptr.get());
		if (pCom)
			pCom->stopTodTimelinePlay();
	}

};
