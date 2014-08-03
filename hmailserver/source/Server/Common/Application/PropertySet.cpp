// Copyright (c) 2010 Martin Knafve / hMailServer.com.  
// http://www.hmailserver.com

#include "stdafx.h"
#include "propertyset.h"
#include "Property.h"
#include "../Util/Crypt.h"


#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif


using namespace std;


namespace HM
{
   PropertySet::PropertySet(void)
   {
   }

   PropertySet::~PropertySet(void)
   {
      
   }

   void 
   PropertySet::Refresh()
   {
      SQLCommand command("select * from hm_settings");
      shared_ptr<DALRecordset> pRS = Application::Instance()->GetDBManager()->OpenRecordset(command);
   
      if (!pRS)
         return;

      std::map<String, shared_ptr<Property> > tmpMap;

      while (!pRS->IsEOF())
      {
         String sPropertyName = pRS->GetStringValue("settingname");
         long lPropertyLong = pRS->GetLongValue("settinginteger");
         String sPropertyString = pRS->GetStringValue("settingstring");

         bool bIsCrypted = false;
         if (IsCryptedProperty_(sPropertyName))
         {
            // De-crypt the string after load from DB.
            sPropertyString = Crypt::Instance()->DeCrypt(sPropertyString, Crypt::ETBlowFish);
            bIsCrypted = true;
         }
      
         shared_ptr<Property> oProperty = shared_ptr<Property> (new Property(sPropertyName, lPropertyLong, sPropertyString));
         if (bIsCrypted)
            oProperty->SetIsCrypted();

         tmpMap[sPropertyName] = oProperty;

         pRS->MoveNext();
      }



      items_ = tmpMap;

      std::map<String, shared_ptr<Property> >::iterator iter = items_.begin();
      std::map<String, shared_ptr<Property> >::iterator iterEnd = items_.end();
      for (; iter != iterEnd; iter++)
      {
         // Trigger an change-event for all options.
         OnPropertyChanged_((*iter).second);
      }
   }

   shared_ptr<Property>
   PropertySet::GetProperty_(const String & sPropertyName)
   {
      std::map<String, shared_ptr<Property> >::iterator iterProperty = items_.find(sPropertyName);
   
      if (iterProperty != items_.end())
         return (*iterProperty).second;

      String sErrorMessage;
      sErrorMessage.Format(_T("The property %s could not be found."), sPropertyName);
      ErrorManager::Instance()->ReportError(ErrorManager::Medium, 5015, "PropertySet::GetProperty_()", sErrorMessage);

      // Property was not found. Create one temporary in memory 
      // to avoid further problems.
      shared_ptr<Property> oProperty = shared_ptr<Property>(new Property());
      return oProperty;
   }

   long
   PropertySet::GetLong(const String &sPropertyName)
   {
      return GetProperty_(sPropertyName)->GetLongValue();
   }

   bool
   PropertySet::GetBool(const String &sPropertyName)
   {
      return GetProperty_(sPropertyName)->GetBoolValue();
   }

   String
   PropertySet::GetString(const String &sPropertyName)
   {
      return GetProperty_(sPropertyName)->GetStringValue();
   }

   void 
   PropertySet::SetLong(const String &sPropertyName, long lValue)
   {
      shared_ptr<Property> pProperty = GetProperty_(sPropertyName);
      bool bChanged = lValue != pProperty->GetLongValue();
      pProperty->SetLongValue(lValue);

      if (bChanged)
         OnPropertyChanged_(pProperty);
   }

   void 
   PropertySet::SetBool(const String &sPropertyName, bool bValue)
   {
      shared_ptr<Property> pProperty = GetProperty_(sPropertyName);
      bool bChanged = bValue != pProperty->GetBoolValue();
      pProperty->SetBoolValue(bValue);

      if (bChanged)
         OnPropertyChanged_(pProperty);
   }

   void 
   PropertySet::SetString(const String &sPropertyName, const String &sValue)
   {
      shared_ptr<Property> pProperty = GetProperty_(sPropertyName);
      bool bChanged = sValue != pProperty->GetStringValue();
      pProperty->SetStringValue(sValue);

      if (bChanged)
         OnPropertyChanged_(pProperty);
   }

   void 
   PropertySet::OnPropertyChanged_(shared_ptr<Property> pProperty)
   {
      // Notify configuration that a setting has changed.
      Configuration::Instance()->OnPropertyChanged(pProperty);
   }

   bool 
   PropertySet::IsCryptedProperty_(const String &sPropertyName)
   {
      if (sPropertyName == PROPERTY_SMTPRELAYER_PASSWORD)
         return true;

      return false;
   }

   bool
   PropertySet::XMLStore(XNode *pBackupNode)
   {
      XNode *pPropertiesNode = pBackupNode->AppendChild(_T("Properties"));
      std::map<String, shared_ptr<Property> >::iterator iterProperty = items_.begin();

      while (iterProperty != items_.end())
      {
         shared_ptr<Property> oProperty = (*iterProperty).second;

         XNode *pNode = pPropertiesNode->AppendChild(String(oProperty->GetName()));

         pNode->AppendAttr(_T("LongValue"), StringParser::IntToString(oProperty->GetLongValue()));
         pNode->AppendAttr(_T("StringValue"), oProperty->GetStringValue());

         iterProperty++;
      }      

      return true;
   }

   bool
   PropertySet::XMLLoad(XNode *pBackupNode)
   {
      XNode *pPropertiesNode = pBackupNode->GetChild(_T("Properties"));
      if (!pPropertiesNode)
         return true;

      for (int i = 0; i < pPropertiesNode->GetChildCount(); i++)
      {
         XNode *pPropertyNode = pPropertiesNode->GetChild(i);
         String sName = pPropertyNode->name;
         String sStringValue = pPropertyNode->GetAttrValue(_T("StringValue"));
         int iLongValue = _ttoi(pPropertyNode->GetAttrValue(_T("LongValue")));

         shared_ptr<Property> pProperty = GetProperty_(sName);
         if (pProperty)
         {
            pProperty->SetStringValue(sStringValue);
            pProperty->SetLongValue(iLongValue);
         }
      }

      return true;
   }
}
