// Author: anhaoxiang
#include "EchoComponentStableHeaders.h"
#include "EchoTimeLineInfo.h"
#include "EchocJsonHelper.h"
#include "EchoActorProFuncImp.h"
#include "EchoRoot.h"
#include "EchoResourceManagerFactory.h"
#include "EchoTimelineComponent.h"
#include "EchoHermiteCurve.h"
#include "EchoHermite.h"

#include "EchoHermiteMacro.h"

namespace Echo {

	
    //TimeLineInfo
	void TimeLineInfo::prepareImpl(void)
	{
		loadTimelineFile(mName.c_str());
	}
	size_t TimeLineInfo::calcMemUsage(void) const
	{
		return 0u;
	}


	void TimeLineInfo::saveTimelineMotionFile(const std::string& filePath, const TimelineMotionVec& motionVec_)
	{
		if (filePath == "" || motionVec_.empty())
			return;
		cJSON* pRoot = cJSON_CreateObject();
		cJSON* motionArray_ = cJSON_CreateArray();
		cJSON_AddItemToObject(pRoot, "motionArray", motionArray_);
		
		cJSON_AddNumberToObject(pRoot,"playType",currentPlayWay);
		for (const auto& motionIter_ : motionVec_)
	    {
			cJSON* motionData_ = cJSON_CreateObject();
			cJSON_AddItemToArray(motionArray_, motionData_);
			cJSON_AddNumberToObject(motionData_, "Type", motionIter_.type);

			cJSON* line_ = cJSON_CreateArray();
			cJSON_AddItemToObject(motionData_, "line", line_);

			for (const auto& iter : motionIter_.motionData)
			{
				cJSON* lineData_ = cJSON_CreateObject();
				cJSON_AddItemToArray(line_, lineData_);
				WriteJSNode(lineData_, "key", iter.first);

				cJSON* value_ = cJSON_CreateObject();
				cJSON_AddItemToObject(lineData_, "value", value_);

				cJSON* pos_ = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "pos", pos_);
				cJSON_AddNumberToObject(pos_, "x", iter.second.value.x);
				cJSON_AddNumberToObject(pos_, "y", iter.second.value.y);
				cJSON_AddNumberToObject(pos_, "z", iter.second.value.z);
				cJSON_AddNumberToObject(pos_, "w", iter.second.value.w);

				cJSON* leftHandTangent_ = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "leftHandTangent", leftHandTangent_);
				cJSON_AddNumberToObject(leftHandTangent_, "x", iter.second.leftHandTangent.x);
				cJSON_AddNumberToObject(leftHandTangent_, "y", iter.second.leftHandTangent.y);
				cJSON_AddNumberToObject(leftHandTangent_, "z", iter.second.leftHandTangent.z);
				cJSON_AddNumberToObject(leftHandTangent_, "w", iter.second.leftHandTangent.w);

				cJSON* rightHandTangent_ = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "rightHandTangent", rightHandTangent_);
				cJSON_AddNumberToObject(rightHandTangent_, "x", iter.second.rightHandTangent.x);
				cJSON_AddNumberToObject(rightHandTangent_, "y", iter.second.rightHandTangent.y);
				cJSON_AddNumberToObject(rightHandTangent_, "z", iter.second.rightHandTangent.z);
				cJSON_AddNumberToObject(rightHandTangent_, "w", iter.second.rightHandTangent.w);

				cJSON* isLeftWeighted = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "isLeftWeighted", isLeftWeighted);
				cJSON_AddNumberToObject(isLeftWeighted, "x", iter.second.isLeftWeighted.x);
				cJSON_AddNumberToObject(isLeftWeighted, "y", iter.second.isLeftWeighted.y);
				cJSON_AddNumberToObject(isLeftWeighted, "z", iter.second.isLeftWeighted.z);

				cJSON* isRightWeighted = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "isRightWeighted", isRightWeighted);
				cJSON_AddNumberToObject(isRightWeighted, "x", iter.second.isRightWeighted.x);
				cJSON_AddNumberToObject(isRightWeighted, "y", iter.second.isRightWeighted.y);
				cJSON_AddNumberToObject(isRightWeighted, "z", iter.second.isRightWeighted.z);

				cJSON* leftWeight = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "leftWeight", leftWeight);
				cJSON_AddNumberToObject(leftWeight, "x", iter.second.leftWeight.x);
				cJSON_AddNumberToObject(leftWeight, "y", iter.second.leftWeight.y);
				cJSON_AddNumberToObject(leftWeight, "z", iter.second.leftWeight.z);

				cJSON* rightWeight = cJSON_CreateObject();
				cJSON_AddItemToObject(value_, "rightWeight", rightWeight);
				cJSON_AddNumberToObject(rightWeight, "x", iter.second.rightWeight.x);
				cJSON_AddNumberToObject(rightWeight, "y", iter.second.rightWeight.y);
				cJSON_AddNumberToObject(rightWeight, "z", iter.second.rightWeight.z);
			}
		}

		char* out = cJSON_Print(pRoot);
		cJSON_Delete(pRoot);
	   
		DataStreamPtr pDataStream(Echo::Root::instance()->CreateDataStream(filePath.data(), "RootMotion"));
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


	void TimeLineInfo::loadTimelineFile(const std::string& filePath)
	{
		if (filePath == "")
			return;
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(filePath.data()));
		if (pDataStream.isNull())
		{
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			     LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "Timeline : File not exit: " + filePath);
			return;
		}
			
		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->read(pData, nSize);
		cJSON* pRoot = cJSON_Parse(pData);
		delete[] pData;
		if (pRoot == nullptr)
			return;
		ReadJSNode(pRoot, "Ver", mVer);

		if (!dataDetection(pRoot, filePath)) return;

		if (Echo::Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			loadEditorData(pRoot);
		loadClientData(pRoot);
		cJSON* AttributeToComponents =  cJSON_GetObjectItem(pRoot, "ActorToComponents");
		if (AttributeToComponents == nullptr)
			loadNewTodData(pRoot);
		else
			loadNewData(pRoot);
		cJSON_Delete(pRoot);
	}

	void TimeLineInfo::saveTimelineFile(const std::string& filePath)
	{
		if (filePath == "")
			return;
		cJSON* pRoot = cJSON_CreateObject();
		WriteJSNode(pRoot,"Ver", 1.3f);
		if (Echo::Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			saveEditorData(pRoot);
		saveClientData(pRoot);
		if (mActorToComponents.empty())
			saveNewTodData(pRoot);
		else
			saveNewData(pRoot);

		char* out = cJSON_Print(pRoot);
		cJSON_Delete(pRoot);

		DataStreamPtr pDataStream(Echo::Root::instance()->CreateDataStream(filePath.data()));
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

	void TimeLineInfo::loadEditorData(cJSON* pRoot)
	{
		cJSON* showComponents_ = cJSON_GetObjectItem(pRoot, "showCompoents");
		if (showComponents_)
		{
			size_t csz = cJSON_GetArraySize(showComponents_);
			showComponents.resize(csz);
			for (int i = 0; i < csz; i++)
			{
				showComponents[i] = cJSON_GetArrayItem(showComponents_, i)->valueint;
			}
		}
		cJSON* AttributeToName_ = cJSON_GetObjectItem(pRoot, "AttributeToName");
		if (AttributeToName_)
		{
			auto names_sz = cJSON_GetArraySize(AttributeToName_);
			AttributeToName.resize(names_sz, std::vector<std::string>(2));
			for (int i = 0; i < names_sz; i++)
			{
				auto namesItem = cJSON_GetArrayItem(AttributeToName_, i);

				auto item0 = cJSON_GetArrayItem(namesItem, 0);
				auto item1 = cJSON_GetArrayItem(namesItem, 1);

				AttributeToName[i][0] = item0->valuestring;
				AttributeToName[i][1] = item1->valuestring;
			}
		}

		int typeTemp;
		ReadJSNode(pRoot, "recordColorTimelineType", typeTemp);
		recordColorTimelineType = static_cast<timelineType>(typeTemp);

		cJSON* Names = cJSON_GetObjectItem(pRoot, "HermiteNames");
		if (Names)
		{
			int sz = cJSON_GetArraySize(Names);
			HermiteNames.resize(sz);
			for (int i = 0; i < sz; i++)
			{
				HermiteNames[i] = cJSON_GetArrayItem(Names, i)->valuestring;
			}
		}

		cJSON* content = cJSON_GetObjectItem(pRoot, "contentSize");
		if (content)
		{
			contentSize_.x = (float)cJSON_GetArrayItem(content, 0)->valuedouble;
			contentSize_.y = (float)cJSON_GetArrayItem(content, 1)->valuedouble;
		}

		ReadJSNode(pRoot, "TimelineName", TimelineName);
		ReadJSNode(pRoot, "mWidthMultiple", mWidthMultiple);
		cJSON* AttributeType_ = cJSON_GetObjectItem(pRoot, "AttributeType");
		if (AttributeType_)
		{
			int sz = cJSON_GetArraySize(AttributeType_);
			AttributeType.resize(sz);
			for (int i = 0; i < sz; i++)
			{
				auto type = cJSON_GetArrayItem(AttributeType_, i);
				int temp;
				ReadJSNode(type, "type", temp);
				AttributeType[i] = static_cast<timelineType>(temp);
			}
		}
	}

	void TimeLineInfo::loadClientData(cJSON* pRoot)
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

		cJSON* pHermiteArray = cJSON_GetObjectItem(pRoot, "HermiteArray");
		if (pHermiteArray)
		{
			for (cJSON* pChild = pHermiteArray->child; nullptr != pChild; pChild = pChild->next)
			{
				EchoHermite::HermiteType hermiteType_ = EchoHermite::Float_Hermite;
				EchoHermite* hermite = nullptr;


				//int flag = false;
				cJSON* pKey = pChild->child;

				std::string hermiteNewType;
				if (mVer > 1.21f)
				{
					ReadJSNode(pKey, "NewHermiteType", hermiteNewType);
					pKey = pKey->next;
				}

				for (; pKey != nullptr; pKey = pKey->next)
				{
					int weight = 0;

					if (!ReadJSNode(pKey, "weight", weight))
					{
						hermiteType_ = static_cast<EchoHermite::HermiteType>(pKey->valueint);

						if (mVer <= 1.21f)
						{
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
								int fragmentType_;
								pKey = pKey->next;
								fragmentType_ = pKey->valueint;
								hermite = new EchoFragmentHermite(static_cast<FragmentType>(fragmentType_));
								hermite->mHermiteType = EchoHermite::Fragment_Hermite;
							}
							else if (hermiteType_ == EchoHermite::String_Hermite)
							{
								hermite = new EchoStringHermite();
								hermite->mHermiteType = EchoHermite::String_Hermite;
							}
						}
						else
						{
						   EchoHermiteFactory* factory_ = EchoHermiteFactortManager::getSingleton().getHermiteFactory(Name(hermiteNewType));
						   if (!factory_)
							   continue;
						   hermite = factory_->createHermite();
						}
						continue;
					}
					if (hermite == nullptr)
						hermite = new EchoFloatHermite();

					HermiteKey* key = hermite->CreateKey();
					key->readJson(pKey);
					hermite->mKeyMap.emplace(weight, key);
				}
				mHermites.emplace_back(hermite);
				mMaxWeight = std::max(mMaxWeight, hermite->GetMaxLength());
				hermite = nullptr;
			}
		}





		cJSON* deltaTime = cJSON_GetObjectItem(pRoot, "mDeltaTime");
		if (deltaTime)
			mDeltaTime = static_cast<float>(deltaTime->valuedouble);

		cJSON* startTime = cJSON_GetObjectItem(pRoot, "startTime");
		if (startTime)
			startSceneTime = static_cast<float>(startTime->valuedouble);
		cJSON* endTime = cJSON_GetObjectItem(pRoot, "endTime");
		if (endTime)
			endSceneTime = static_cast<float>(endTime->valuedouble);
		cJSON* currentPlayWay_ = cJSON_GetObjectItem(pRoot, "currentPlayWay");
		if (currentPlayWay_)
		{
			currentPlayWay = currentPlayWay_->valueint;
		}
		cJSON* AttributeName_ = cJSON_GetObjectItem(pRoot, "AttributeNames");
		if (AttributeName_)
		{
			int sz = cJSON_GetArraySize(AttributeName_);
			AttributeName.resize(sz);
			for (int i = 0; i < sz; i++)
			{
				auto name = cJSON_GetArrayItem(AttributeName_, i);
				ReadJSNode(name, "AttributeName", AttributeName[i]);
			}
		}

		cJSON* AttributeBegin_ = cJSON_GetObjectItem(pRoot, "AttributeBegin");
		if (AttributeBegin_)
		{
			int sz = cJSON_GetArraySize(AttributeBegin_);
			AttributeBegin.resize(sz);
			for (int i = 0; i < sz; i++)
			{
				auto st = cJSON_GetArrayItem(AttributeBegin_, i);
				ReadJSNode(st, "start", AttributeBegin[i]);
			}
		}

		cJSON* AttributeEnd_ = cJSON_GetObjectItem(pRoot, "AttributeEnd");
		if (AttributeEnd_)
		{
			int sz = cJSON_GetArraySize(AttributeEnd_);
			AttributeEnd.resize(sz);
			for (int i = 0; i < sz; i++)
			{
				auto en = cJSON_GetArrayItem(AttributeEnd_, i);
				ReadJSNode(en, "end", AttributeEnd[i]);
			}
		}

		auto orderCnt_ = cJSON_GetObjectItem(pRoot, "OrderCnt");
		if (orderCnt_) {
			OrderCnt = orderCnt_->valueint;
		}
		ReadJSNode(pRoot, "bPlayScene", bPlayScene);

		//ReadJSNode(pRoot, "RootMotionActorName", mRootMotionData.actorName);
		ReadJSNode(pRoot, "RootMotionComName", mRootMotionData.comName);
	}

	void TimeLineInfo::loadNewData(cJSON* pRoot)
	{
		cJSON* actorIdMap_ = cJSON_GetObjectItem(pRoot, "ActorToId");
		if (actorIdMap_)
		{
	        for (auto& actIdIter = actorIdMap_->child; actIdIter != nullptr; actIdIter = actIdIter->next)
			{
				std::string actorName_;
				int actorId_;
				ReadJSNode(actIdIter, "ActorName", actorName_);
				ReadJSNode(actIdIter, "ActorId", actorId_);
				mActorToId[actorName_] = actorId_;
			}
		}

		cJSON* AttributeToComponents = cJSON_GetObjectItem(pRoot, "ActorToComponents");
		if (AttributeToComponents)
		{
			for (auto actorIter = AttributeToComponents->child; actorIter != nullptr; actorIter = actorIter->next)
			{
				mActorToComponents.insert(std::make_pair(actorIter->string, std::vector<std::string>()));
				if (mActorToId.find(actorIter->string) == mActorToId.end())
				{
					mActorToId[actorIter->string] = -1;
				}
				for (auto comIter = actorIter->child; comIter != nullptr; comIter = comIter->next)
				{
					mActorToComponents[actorIter->string].emplace_back(comIter->string);
					mComponentToAttributes.insert(std::make_pair(comIter->string, std::vector<attributeData>()));
					for (auto attrIter = comIter->child; attrIter != nullptr; attrIter = attrIter->next)
					{
						attributeData tempData;

						tempData.attributeName = attrIter->string;
						ReadJSNode(attrIter, "attribute_st", tempData.st);
						ReadJSNode(attrIter, "attribute_end", tempData.en);
#ifdef ECHO_EDITOR		
						ReadJSNode(attrIter, "attribute_ch_name", tempData.text_name);
						ReadJSNode(attrIter, "isShow", tempData.isShow);
#endif
						ReadJSNode(attrIter, "isColor", tempData.isColor);
						ReadJSNode(attrIter, "isCamera", tempData.isCamera);
						ReadJSNode(attrIter, "isNoInterpolate", tempData.isNoInterpolate);
						ReadJSNode(attrIter, "isAnimation", tempData.isAnimation);
						ReadJSNode(attrIter, "isTruncation", tempData.isTruncation);
						ReadJSNode(attrIter, "isQuaternion", tempData.isQuaternion);

						if (tempData.en - tempData.st == 4)
						{
							const std::string& attrNameStr_ = tempData.attributeName.c_str();
							if (attrNameStr_.find("Color") != std::string::npos)
								tempData.isColor = 1;
						}
						if (tempData.en - tempData.st == 1)
						{
							const std::string& attrNameStr_ = tempData.attributeName.c_str();
							if (attrNameStr_ == "cameraComUseState")
							    tempData.isCamera = true;
						}
						mComponentToAttributes[comIter->string].emplace_back(tempData);
					}
				}
			}
		}
#ifdef ECHO_EDITOR
		ReadJSNode(pRoot, "CameraMgrIdx", mCameraMgrIdx);
		ReadJSNode(pRoot, "mCameraFrameToIdx", mCameraFrameToIdx);
#endif
	}

	void TimeLineInfo::loadNewTodData(cJSON* pRoot)
	{
		cJSON* ComponentToAttribute = cJSON_GetObjectItem(pRoot, "ComponentToAttribute");
		if (ComponentToAttribute)
		{
			for (auto comIter = ComponentToAttribute->child; comIter != nullptr; comIter = comIter->next)
			{
				
				mComponentToAttributes.insert(std::make_pair(comIter->string, std::vector<attributeData>()));
				for (auto attrIter = comIter->child; attrIter != nullptr; attrIter = attrIter->next)
				{
					attributeData tempData;
					tempData.attributeName = attrIter->string;
					ReadJSNode(attrIter, "attribute_st", tempData.st);
					ReadJSNode(attrIter, "attribute_end", tempData.en);
#ifdef ECHO_EDITOR		
					ReadJSNode(attrIter, "attribute_ch_name", tempData.text_name);
					ReadJSNode(attrIter, "isShow", tempData.isShow);
#endif
					ReadJSNode(attrIter, "isColor", tempData.isColor);
					ReadJSNode(attrIter, "isCamera", tempData.isCamera);
					ReadJSNode(attrIter, "isNoInterpolate", tempData.isNoInterpolate);
					ReadJSNode(attrIter, "isAnimation", tempData.isAnimation);
					ReadJSNode(attrIter, "isTruncation", tempData.isTruncation);
					ReadJSNode(attrIter, "isQuaternion", tempData.isQuaternion);
					if (tempData.en - tempData.st == 4)
					{
						const std::string& attrNameStr_ = tempData.attributeName.c_str();
						if (attrNameStr_.find("Color") != std::string::npos)
							tempData.isColor = 1;
					}
					mComponentToAttributes[comIter->string].emplace_back(tempData);
				}
			}
		}
	}

	void TimeLineInfo::saveNewData(cJSON* pRoot)
	{
		cJSON* ActorToComponents = cJSON_CreateObject();
		cJSON_AddItemToObject(pRoot, "ActorToComponents", ActorToComponents);
		for (const auto& actorIter : mActorToComponents)
		{
			cJSON* actorMaps = cJSON_CreateObject();
			cJSON_AddItemToObject(ActorToComponents, actorIter.first.data(), actorMaps);
			for (const auto& comIter : actorIter.second)
			{
				cJSON* comMaps = cJSON_CreateObject();
				cJSON_AddItemToObject(actorMaps, comIter.c_str(), comMaps);
				for (const auto& attrIter : mComponentToAttributes[comIter])
				{
					cJSON* attribute = cJSON_CreateObject();
					cJSON_AddItemToObject(comMaps, attrIter.attributeName.c_str(), attribute);
					cJSON_AddNumberToObject(attribute, "attribute_st", attrIter.st);
					cJSON_AddNumberToObject(attribute, "attribute_end", attrIter.en);
#ifdef ECHO_EDITOR		
					cJSON_AddStringToObject(attribute, "Attribute_ch_name", attrIter.text_name.data());
					cJSON_AddBoolToObject(attribute, "isShow", attrIter.isShow);
#endif
					WriteJSNode(attribute,"isColor",attrIter.isColor);
					WriteJSNode(attribute,"isCamera",attrIter.isCamera);
					WriteJSNode(attribute,"isNoInterpolate", attrIter.isNoInterpolate);
					WriteJSNode(attribute, "isAnimation", attrIter.isAnimation);
					WriteJSNode(attribute, "isTruncation", attrIter.isTruncation);
					WriteJSNode(attribute, "isQuaternion", attrIter.isQuaternion);
				}
			}
		}

		cJSON* actorIdMap_ = cJSON_CreateArray();
		cJSON_AddItemToObject(pRoot, "ActorToId", actorIdMap_);
		for (const auto& actorIdIter : mActorToId)
		{
			cJSON* actorItem_ = cJSON_CreateObject();
			cJSON_AddStringToObject(actorItem_, "ActorName", actorIdIter.first.data());
			cJSON_AddNumberToObject(actorItem_, "ActorId", actorIdIter.second);
			cJSON_AddItemToArray(actorIdMap_, actorItem_);
		}
#ifdef ECHO_EDITOR
		WriteJSNode(pRoot, "CameraMgrIdx", mCameraMgrIdx);
	    WriteJSNode(pRoot, "mCameraFrameToIdx", mCameraFrameToIdx);
#endif
	}

	void TimeLineInfo::saveNewTodData(cJSON* pRoot)
	{
		cJSON* componentToAttribute_ = cJSON_CreateObject();
		cJSON_AddItemToObject(pRoot, "ComponentToAttribute", componentToAttribute_);
		for (const auto& comIter : mComponentToAttributes)
		{
			cJSON* comMaps = cJSON_CreateObject();
			cJSON_AddItemToObject(componentToAttribute_, comIter.first.c_str(), comMaps);
			for (const auto& attrIter : comIter.second)
			{
				cJSON* attribute = cJSON_CreateObject();
				cJSON_AddItemToObject(comMaps, attrIter.attributeName.c_str(), attribute);
				cJSON_AddNumberToObject(attribute, "attribute_st", attrIter.st);
				cJSON_AddNumberToObject(attribute, "attribute_end", attrIter.en);
#ifdef ECHO_EDITOR		
				cJSON_AddStringToObject(attribute, "Attribute_ch_name", attrIter.text_name.data());
				cJSON_AddBoolToObject(attribute, "isShow", attrIter.isShow);
#endif
				WriteJSNode(attribute, "isColor", attrIter.isColor);
				WriteJSNode(attribute, "isCamera", attrIter.isCamera);
				WriteJSNode(attribute, "isNoInterpolate", attrIter.isNoInterpolate);
				WriteJSNode(attribute, "isAnimation", attrIter.isAnimation);
				WriteJSNode(attribute, "isTruncation", attrIter.isTruncation);
				WriteJSNode(attribute, "isQuaternion", attrIter.isQuaternion);
			}

		}
	}

	void TimeLineInfo::saveEditorData(cJSON* pRoot)
	{
		if (showComponents.size() != 0)
		{
			cJSON* showComponents_ = cJSON_CreateIntArray(&showComponents[0], static_cast<int>(showComponents.size()));
			cJSON_AddItemToObject(pRoot, "showCompoents", showComponents_);
		}
		cJSON* AttributeToName_ = cJSON_CreateArray();
		int names_sz = static_cast<int>(AttributeToName.size());
		for (int i = 0; i < names_sz; i++)
		{
			cJSON* namesItem = cJSON_CreateArray();
			int item_sz = static_cast<int>(AttributeToName[i].size());
			for (int j = 0; j < item_sz; j++)
			{
				auto item = cJSON_CreateString(AttributeToName[i][j].data());
				cJSON_AddItemToArray(namesItem, item);
			}
			cJSON_AddItemToArray(AttributeToName_, namesItem);
		}
		cJSON_AddItemToObject(pRoot, "AttributeToName", AttributeToName_);
		cJSON_AddNumberToObject(pRoot, "recordColorTimelineType", recordColorTimelineType);
		cJSON* Names = cJSON_CreateArray();
		for (int i = 0; i < HermiteNames.size(); i++) {
			auto name = cJSON_CreateString(HermiteNames[i].data());
			cJSON_AddItemToArray(Names, name);
		}
		cJSON_AddItemToObject(pRoot, "HermiteNames", Names);
		cJSON* content = cJSON_CreateFloatArray(contentSize_.ptr(), 2);
		cJSON_AddItemToObject(pRoot, "contentSize", content);
		cJSON_AddNumberToObject(pRoot, "mWidthMultiple", mWidthMultiple);
		cJSON_AddStringToObject(pRoot, "TimelineName", TimelineName.data());

		cJSON* AttributeType_ = cJSON_CreateArray();
		for (auto& item : AttributeType)
		{
			auto type = cJSON_CreateObject();
			cJSON_AddNumberToObject(type, "type", item);
			cJSON_AddItemToArray(AttributeType_, type);
		}
		cJSON_AddItemToObject(pRoot, "AttributeType", AttributeType_);

	}

	void TimeLineInfo::saveClientData(cJSON* pRoot)
	{
		cJSON* pHermiteArray = cJSON_CreateArray();
		for (auto hermite : mHermites)
		{
			cJSON* pHermite = cJSON_CreateArray();

			cJSON* newType = cJSON_CreateObject();
			cJSON_AddStringToObject(newType, "NewHermiteType", hermite->getType().c_str());
			cJSON_AddItemToArray(pHermite,newType);


			cJSON_AddNumberToObject(pHermite, "mHermiteType", hermite->mHermiteType);
			if (hermite->mHermiteType == EchoHermite::Fragment_Hermite)
			{
				cJSON_AddNumberToObject(pHermite, "FragmentType", static_cast<EchoFragmentHermite*>(hermite)->getFragmentType());
			}

			cJSON_AddItemToArray(pHermiteArray, pHermite);
			for (auto hKey : hermite->mKeyMap)
			{
				cJSON* pHermiteKey = cJSON_CreateObject();

				cJSON_AddNumberToObject(pHermiteKey, "weight", hKey.first);
				hKey.second->writeJson(pHermiteKey);
				
				cJSON_AddItemToArray(pHermite, pHermiteKey);
			}
		}
		cJSON_AddItemToObject(pRoot, "HermiteArray", pHermiteArray);
		cJSON_AddNumberToObject(pRoot, "startTime", startSceneTime);
		cJSON_AddNumberToObject(pRoot, "endTime", endSceneTime);
		cJSON_AddNumberToObject(pRoot, "mDeltaTime", mDeltaTime);
		cJSON_AddNumberToObject(pRoot, "currentPlayWay", currentPlayWay);
		cJSON* AttributeName_ = cJSON_CreateArray();
		for (auto& item : AttributeName)
		{
			auto str = cJSON_CreateObject();
			cJSON_AddStringToObject(str, "AttributeName", item.data());
			cJSON_AddItemToArray(AttributeName_, str);
		}
		cJSON_AddItemToObject(pRoot, "AttributeNames", AttributeName_);

		cJSON* AttributeBegin_ = cJSON_CreateArray();
		for (auto& item : AttributeBegin)
		{
			auto st = cJSON_CreateObject();
			cJSON_AddNumberToObject(st, "start", item);
			cJSON_AddItemToArray(AttributeBegin_, st);
		}
		cJSON_AddItemToObject(pRoot, "AttributeBegin", AttributeBegin_);

		cJSON* AttributeEnd_ = cJSON_CreateArray();
		for (auto& item : AttributeEnd)
		{
			auto en = cJSON_CreateObject();
			cJSON_AddNumberToObject(en, "end", item);
			cJSON_AddItemToArray(AttributeEnd_, en);
		}
		cJSON_AddItemToObject(pRoot, "AttributeEnd", AttributeEnd_);
		cJSON_AddNumberToObject(pRoot, "OrderCnt", OrderCnt);
		cJSON_AddBoolToObject(pRoot, "bPlayScene", bPlayScene);

		//cJSON_AddStringToObject(pRoot, "RootMotionActorName", mRootMotionData.actorName.data());
		cJSON_AddStringToObject(pRoot, "RootMotionComName", mRootMotionData.comName.data());

	}

	bool TimeLineInfo::dataDetection(cJSON* pRoot, const std::string& filePath)
	{
		if (mVer <= 1.21f)
			return true;

		cJSON* pHermiteArray = cJSON_GetObjectItem(pRoot, "HermiteArray");
		int pHermiteCount = cJSON_GetArraySize(pHermiteArray);
		int pAttributeCount = 0;
		cJSON* AttributeToComponents = cJSON_GetObjectItem(pRoot, "ActorToComponents");
		if (AttributeToComponents)
		{
			for (auto actorIter = AttributeToComponents->child; actorIter != nullptr; actorIter = actorIter->next)
				for (auto comIter = actorIter->child; comIter != nullptr; comIter = comIter->next)
					for (auto attrIter = comIter->child; attrIter != nullptr; attrIter = attrIter->next)
					{
						float st = 0;
						float en = 0;
						ReadJSNode(attrIter, "attribute_st", st);
						ReadJSNode(attrIter, "attribute_end", en);
						pAttributeCount = pAttributeCount < (int)en ? (int)en : pAttributeCount;
					}
			int mCameraMgrIdx = 0;
			ReadJSNode(pRoot, "CameraMgrIdx", mCameraMgrIdx);
			pAttributeCount = pAttributeCount <= mCameraMgrIdx ? mCameraMgrIdx + 1 : pAttributeCount;

			if (pAttributeCount != pHermiteCount)
			{
				LogManager::instance()->logMessage("[TimelineFile] Loading incorrect data because the quantity of data does not match : " + filePath);
				return false;
			}
		}
		else
		{
			cJSON* ComponentToAttribute = cJSON_GetObjectItem(pRoot, "ComponentToAttribute");
			if (ComponentToAttribute)
			{
				for (auto comIter = ComponentToAttribute->child; comIter != nullptr; comIter = comIter->next)
					for (auto attrIter = comIter->child; attrIter != nullptr; attrIter = attrIter->next)
					{
						float st = 0;
						float en = 0;
						ReadJSNode(attrIter, "attribute_st", st);
						ReadJSNode(attrIter, "attribute_end", en);
						pAttributeCount = pAttributeCount < (int)en ? (int)en : pAttributeCount;
					}
			}

			if (pAttributeCount != pHermiteCount)
			{
				LogManager::instance()->logMessage("[TimelineFile] Loading incorrect data because the quantity of data does not match : " + filePath);
				return false;
			}
		}
		return true;
	}

	float TimeLineInfo::getTimeLength()
	{
		return endSceneTime - startSceneTime;
	}

	TimeLineInfo::TimeLineInfo(ResourceManagerBase* creator, const Name& actFileName, bool isManual, ManualResourceLoader* loader)
		: ResourceBase(creator, actFileName, isManual, loader)
	{

	}

	TimeLineInfo::~TimeLineInfo()
	{
		unload();
	}



	//TimeLineInfoManager

	TimeLineInfoManager::TimeLineInfoManager()
		: ResourceManagerBase()
	{
		mResourceType = "TimeLineInfo";
		ResourceManagerFactory::instance()->registerResourceManager(mResourceType, this);
		
		REGISTERHERMITEFACTORY(Float)
		REGISTERHERMITEFACTORY(String)
		REGISTERHERMITEFACTORY(Fragment)
		REGISTERHERMITEFACTORY(Bool)

		ProtocolComponentSystem::instance()->callProtocolFunc(PFID_RegisterHermiteKeyType, nullptr, 0);
	}

	TimeLineInfoManager::~TimeLineInfoManager()
	{
		ResourceManagerFactory::instance()->unregisterResourceManager(mResourceType);
	}

	TimeLineInfoPtr TimeLineInfoManager::createManual(const Name& name, ManualResourceLoader* loader)
	{
		TimeLineInfoPtr newTimelineInfo = createResource(name, true, loader);
		return newTimelineInfo;
	}

	ResourceBase* TimeLineInfoManager::createImpl(const Name& name, bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParams)
	{
		TimeLineInfo* timelineInfo = new TimeLineInfo(this, name, isManual, loader);
		return timelineInfo;
	}

	//TimeLineInfoLoadingTood

	void TimeLineInfoLoadingTood::registerTimelineComData(const Name& fileName_, const EchoWeakPtr<TimelineComponent>& timelinePtr_, bool isResetTime_,TimelineComListener* listener)
	{
		if (listener == nullptr)
			return;
		TimelineComData data_;
		data_.timelineComPtr = timelinePtr_;
		data_.isReSetTime = isResetTime_;
		data_.timelineListener = listener;
		mTimelineComDataMap.insert(std::make_pair(fileName_, data_));
	}

	void TimeLineInfoLoadingTood::unRegisterTimelineComData(TimelineComListener* listener)
	{
		if (listener == nullptr)
			return;
		for (auto& iter : mTimelineComDataMap)
		{
			if (iter.second.timelineListener == listener)
				iter.second.timelineListener = nullptr;
		}
	}

	void TimeLineInfoLoadingTood::refreshMissingTimeline(const ActorPtr& actorPtr_)
	{
		if (!actorPtr_.expired())
		{
			if (mMissingTimelineMap.contains(actorPtr_))
			{
				std::vector<TimelineComponent*> delVec;
				for (TimelineComponent* timeCom_ : mMissingTimelineMap[actorPtr_])
				{
					if (timeCom_)
					{
						bool isExitMissingActor_ = timeCom_->refreshExitAttributeActor();
						bool isExitMissingCom_ = timeCom_->refreshExitAttributeCom();
						if (!(isExitMissingActor_ || isExitMissingCom_))
							delVec.emplace_back(timeCom_);
					}
				}
				for (int i = 0; i < delVec.size(); ++i)
					mMissingTimelineMap[actorPtr_].erase(delVec[i]);

				if (mMissingTimelineMap[actorPtr_].empty())
					mMissingTimelineMap.erase(actorPtr_);
			}
		}
		else
		{
			if(mMissingTimelineMap.contains(actorPtr_))
			   mMissingTimelineMap.erase(actorPtr_);
		}
	}

	void TimeLineInfoLoadingTood::addMissingTimeline(const ActorPtr& actPtr_,TimelineComponent* timelineCom_)
	{
		if (!actPtr_.expired() && timelineCom_)
		{
			mMissingTimelineMap[actPtr_].insert(timelineCom_);
		}
	}

	void TimeLineInfoLoadingTood::eraseMissingTimeline(TimelineComponent* timelineCom_)
	{
	
		for (auto& actIter_ : mMissingTimelineMap)
		{
			if (!actIter_.first.expired())
			{
	            for (TimelineComponent* timeCom_ : actIter_.second)
				{
					if(timeCom_ == timelineCom_)
					{
						actIter_.second.erase(timeCom_);
						if (actIter_.second.empty())
							mMissingTimelineMap.erase(actIter_.first);
						return;
					}
				}
			}
		}
	}

	void TimeLineInfoLoadingTood::TimeLineInfoResourceHandleListener::requestResource(const Name& resName, const Name& resType, ResourceLoadPriority priority)
	{
		BackgroundProcessTicket ticket = ResourceBackgroundQueue::instance()->prepare(resType, resName, priority, false, nullptr, nullptr, this);
		mMapTickets.insert(ticket);
	}

	void TimeLineInfoLoadingTood::TimeLineInfoResourceHandleListener::clearRequest()
	{
		auto ite = mMapTickets.end();
		for (auto it = mMapTickets.begin(); it != ite; ++it)
		{
			ResourceBackgroundQueue::instance()->abortRequest(*it);
		}
		mMapTickets.clear();
	}

	void TimeLineInfoLoadingTood::TimeLineInfoResourceHandleListener::operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr)
	{
		auto it = mMapTickets.find(ticket);
		if (it == mMapTickets.end())
			return;
		if (!request.result.error && !resourcePtr.isNull())
		{
              resourcePtr->load();
			  TimeLineInfoLoadingTood::instance()->timelineInfoLoadCompleted(request.name,static_cast<TimeLineInfoPtr>(resourcePtr));
		}
		else
		{
			std::string logInfo = "";
			logInfo.append("TimeLineFile  : Load false ").append(request.name.c_str());
			LogManager::getSingleton().logMessage(Echo::LML_NORMAL, logInfo.data());
		}
		mMapTickets.erase(it);
			
	}
	void TimeLineInfoLoadingTood::requestResource(const Name& resName, const Name& resType, ResourceLoadPriority priority)
	{
		if (mResourceHandleListener == nullptr)
			mResourceHandleListener = new TimeLineInfoResourceHandleListener();
		mResourceHandleListener->requestResource(resName, resType, priority);
	}

	void TimeLineInfoLoadingTood::timelineInfoLoadCompleted(const Name& resName,const TimeLineInfoPtr& TimelineInfo)
	{
		
		std::multimap<Name, TimelineComData>::const_iterator iter = mTimelineComDataMap.find(resName);
		while (iter != mTimelineComDataMap.end())
		{
			if (iter != mTimelineComDataMap.end())
			{
				if (iter->second.timelineListener != nullptr)
				{
					iter->second.timelineListener->setTimelineInfo(TimelineInfo);
					iter->second.timelineListener->TimelineInfoLoadComplete(iter->second);
				}
			}
			mTimelineComDataMap.erase(iter);
			iter = mTimelineComDataMap.find(resName);
		}
	}

	TimeLineInfoLoadingTood::~TimeLineInfoLoadingTood()
	{
		if (mResourceHandleListener)
		{
			mResourceHandleListener->clearRequest();
			SAFE_DELETE(mResourceHandleListener);
		}
		mMissingTimelineMap.clear();
	}

	TimeLineInfoPtr TimeLineInfoLoadingTood::loadFile(const Name& filePath, bool sync)
	{
		if (sync)
		{
			TimeLineInfoPtr p_Timeline;
			try {
				p_Timeline = TimeLineInfoManager::instance()->load(filePath);
			}
			catch (...)
			{
				p_Timeline = TimeLineInfoManager::instance()->createManual(filePath);
				if (p_Timeline.isNull())
					LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "Timeline : File not exit : Timelineinfo : " + filePath.toString());
				else
					p_Timeline->load();
			}
			return p_Timeline;
		}
		else
		{
			ResourceBasePtr pResource = TimeLineInfoManager::instance()->getByName(filePath);
			if (!pResource.isNull())
			{
				if (pResource->isLoaded())
				{
				}
				else if (pResource->isPrepared())
				{
					pResource->load();
				}
				else
				{
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
					requestResource(Name(filePath), TimeLineInfoManager::instance()->getResourceType(), priority);
				}
			}
			else
			{
				pResource = TimeLineInfoManager::instance()->createResource(filePath,false);
				ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
				requestResource(Name(filePath), TimeLineInfoManager::instance()->getResourceType(), priority);
			}
			return static_cast<TimeLineInfoPtr>(pResource);
		}
	}
	bool TimeLineInfoLoadingTood::checkTimelineInfo(TimeLineInfoPtr& pTimelineInfo)
	{
		if (pTimelineInfo.isNull())
			return false;
		if (pTimelineInfo->isLoaded())
		{
			return true;
		}
		else
		{
			std::string logInfo = "";
			logInfo.append("TimeLineFile  : is Loading ").append(pTimelineInfo->getName().c_str());
			LogManager::getSingleton().logMessage(Echo::LML_NORMAL, logInfo.data());
			//pTimelineInfo->load();
		}
		return false;
	}
};
