#ifndef _EchoHermiteMacro_h__
#define _EchoHermiteMacro_h__

#include "EchoComponentPrerequisites.h"
#include "EchoHermite.h"

namespace Echo {

	template<class T>
	class  HermiteTemplateStringKey : public HermiteStringKey
	{
	public:
		HermiteTemplateStringKey(const T& iValue)
			:mValue(iValue)
		{
		}
		HermiteTemplateStringKey()  {}
		virtual ~HermiteTemplateStringKey() override {}
		inline virtual void Copy(HermiteKey* key) override
		{
			HermiteStringKey::Copy(key);
			HermiteTemplateStringKey* key_ = static_cast<HermiteTemplateStringKey*>(key);
			key_->mValue = mValue;
		}
		inline virtual void writeJson(cJSON* pRoot) override
		{
			HermiteStringKey::writeJson(pRoot);
			writeHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
		inline virtual void readJson(cJSON* pRoot) override
		{
			HermiteStringKey::readJson(pRoot);
			readHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
	public:
		T			 mValue = T();
	};

	template<class T>
	class  HermiteTemplateFloatKey : public HermiteFloatKey
	{
	public:
		HermiteTemplateFloatKey(const T& iValue)
			:mValue(iValue) {}

		HermiteTemplateFloatKey() {}
		virtual ~HermiteTemplateFloatKey() override{}
		inline virtual void Copy(HermiteKey* key) override
		{
			HermiteFloatKey::Copy(key);
			HermiteTemplateFloatKey* key_ = static_cast<HermiteTemplateFloatKey*>(key);
			key_->mValue = mValue;
		}
		inline virtual void writeJson(cJSON* pRoot) override
		{
			HermiteFloatKey::writeJson(pRoot);
			writeHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
		inline virtual void readJson(cJSON* pRoot) override
		{
			HermiteFloatKey::readJson(pRoot);
			readHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
	public:
		T			 mValue = T();
	};


	template<class T>
	class  HermiteTemplateBoolKey : public HermiteBoolKey
	{
	public:
		HermiteTemplateBoolKey(const T& iValue)
			:mValue(iValue) {}

		HermiteTemplateBoolKey() {}
		virtual ~HermiteTemplateBoolKey() override{}
		inline virtual void Copy(HermiteKey* key) override
		{
			HermiteBoolKey::Copy(key);
			HermiteTemplateBoolKey* key_ = static_cast<HermiteTemplateBoolKey*>(key);
			key_->mValue = mValue;
		}
		inline virtual void writeJson(cJSON* pRoot) override
		{
			HermiteBoolKey::writeJson(pRoot);
			writeHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
		inline virtual void readJson(cJSON* pRoot) override
		{
			HermiteBoolKey::readJson(pRoot);
			readHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
	public:
		T			 mValue = T();
	};

	template<class T>
	class  HermiteTemplateFragmentKey : public HermiteFragmentKey
	{
	public:
		HermiteTemplateFragmentKey(const T& iValue)
			:mValue(iValue) {}

		HermiteTemplateFragmentKey() {}
		virtual ~HermiteTemplateFragmentKey() override {}
		inline virtual void Copy(HermiteKey* key) override
		{
			HermiteFragmentKey::Copy(key);
			HermiteTemplateFragmentKey* key_ = static_cast<HermiteTemplateFragmentKey*>(key);
			key_->mValue = mValue;
		}
		inline virtual void writeJson(cJSON* pRoot) override
		{
			HermiteFragmentKey::writeJson(pRoot);
			writeHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
		inline virtual void readJson(cJSON* pRoot) override
		{
			HermiteFragmentKey::readJson(pRoot);
			readHermiteKeyJson<T>(pRoot, "templateValue", mValue);
		}
	public:
		T			 mValue = T();
	};




#ifndef DECLARE_HERMITE_TYPE
#define DECLARE_HERMITE_TYPE(HERMITENAME,VALUE_TYPE,HERMITE_TYPE)\
	DECLARE_HERMITE_TYPE_H(Echo##HERMITENAME##Hermite)\
public:\
	Echo##HERMITENAME##Hermite(){}\
	virtual ~Echo##HERMITENAME##Hermite() override {}\
	virtual  Echo::HermiteKey* CreateKey() override { return new  Echo::HermiteTemplate##HERMITE_TYPE##Key<VALUE_TYPE>();}\
    Echo::HermiteTemplate##HERMITE_TYPE##Key<VALUE_TYPE>* Get##HERMITENAME##Key( Echo::HermiteKey* key) { return static_cast <Echo::HermiteTemplate##HERMITE_TYPE##Key<VALUE_TYPE>*>(key);}
#endif



#ifndef DECLARE_ECHO_HERMITE_STRING
#define DECLARE_ECHO_HERMITE_STRING(HERMITE_NAME,VALUE_TYPE, EXPORT_NAME)\
        class   EXPORT_NAME  Echo##HERMITE_NAME##Hermite : public Echo::EchoStringHermite\
        {\
            DECLARE_HERMITE_TYPE(HERMITE_NAME,VALUE_TYPE,String)\
        };\
       DEFINEHERMITECOMFACTORY(HERMITE_NAME)
#endif

#ifndef DECLARE_ECHO_HERMITE_FLOAT
#define DECLARE_ECHO_HERMITE_FLOAT(HERMITE_NAME,VALUE_TYPE, EXPORT_NAME)\
        class   EXPORT_NAME  Echo##HERMITE_NAME##Hermite : public Echo::EchoFloatHermite\
        {\
            DECLARE_HERMITE_TYPE(HERMITE_NAME,VALUE_TYPE,Float)\
        };\
       DEFINEHERMITECOMFACTORY(HERMITE_NAME)
#endif

#ifndef DECLARE_ECHO_HERMITE_BOOL
#define DECLARE_ECHO_HERMITE_BOOL(HERMITE_NAME,VALUE_TYPE, EXPORT_NAME)\
        class   EXPORT_NAME  Echo##HERMITE_NAME##Hermite : public Echo::EchoBoolHermite\
        {\
            DECLARE_HERMITE_TYPE(HERMITE_NAME,VALUE_TYPE,Bool)\
        };\
       DEFINEHERMITECOMFACTORY(HERMITE_NAME)
#endif

#ifndef DECLARE_ECHO_HERMITE_FRAGMENT
#define DECLARE_ECHO_HERMITE_FRAGMENT(HERMITE_NAME,VALUE_TYPE, EXPORT_NAME)\
        class   EXPORT_NAME  Echo##HERMITE_NAME##Hermite : public Echo::EchoFragmentHermite\
        {\
            DECLARE_HERMITE_TYPE(HERMITE_NAME,VALUE_TYPE,Fragment)\
        };\
       DEFINEHERMITECOMFACTORY(HERMITE_NAME)
#endif		

	//DECLARE_ECHO_HERMITE_STRING(test, float, )

	template<class T>
	void   writeHermiteKeyJson(cJSON* pRoot, const std::string& name, const T& value)
	{
		WriteJSNode(pRoot, name.c_str(), value);
	}

	template<class T>
	void    readHermiteKeyJson(cJSON* pRoot, const std::string& name, T& value)
	{
		ReadJSNode(pRoot, name.c_str(), value);
	}


	////测试
	//DECLARE_ECHO_HERMITE_STRING(Light,float, _EchoComponentExport)
}
#endif
