#pragma once

#include "Foundation/Reflect/Element.h"
#include "Core/Scene/CameraPreferences.h"

namespace Helium
{
    namespace Core
    {
        namespace GeometryModes
        {
            enum GeometryMode
            {
                None,
                Render,
                Collision,
                Pathfinding,
                Count
            };

            static void GeometryModeEnumerateEnum( Reflect::Enumeration* info )
            {
                info->AddElement(GeometryModes::None, TXT( "GeometryModes::None" ) ); 
                info->AddElement(GeometryModes::Render, TXT( "GeometryModes::Render" ) ); 
                info->AddElement(GeometryModes::Collision, TXT( "GeometryModes::Collision" ) ); 
                info->AddElement(GeometryModes::Pathfinding, TXT( "GeometryModes::Pathfinding" ) ); 
                info->AddElement(GeometryModes::Count, TXT( "GeometryModes::Count" ) ); 
            }
        }

        typedef GeometryModes::GeometryMode GeometryMode;

        namespace ViewColorModes
        {
            enum ViewColorMode
            {
                Layer,
                Type,
                Scene,
            };
            static void ViewColorModeEnumerateEnum( Reflect::Enumeration* info )
            {
                info->AddElement( Layer, TXT( "Layer" ) );
                info->AddElement( Type, TXT( "Type" ) );
                info->AddElement( Scene, TXT( "Scene" ) );
            }
        }
        typedef ViewColorModes::ViewColorMode ViewColorMode;

        class CORE_API ViewportPreferences : public Reflect::ConcreteInheritor< ViewportPreferences, Reflect::Element >
        {
        public: 
            ViewportPreferences(); 

            ViewColorMode GetColorMode() const;
            void SetColorMode( ViewColorMode mode );
            const Reflect::Field* ColorModeField() const;

            CameraMode           m_CameraMode; 
            GeometryMode         m_GeometryMode; 
            V_CameraPreferences  m_CameraPrefs; // do not use m_CameraMode as an index!

            bool                 m_Highlighting; 
            bool                 m_AxesVisible; 
            bool                 m_GridVisible; 
            bool                 m_BoundsVisible; 
            bool                 m_StatisticsVisible; 

        private:
            ViewColorMode        m_ColorMode;

        public:
            static void EnumerateClass( Reflect::Compositor<ViewportPreferences>& comp )
            {
                comp.AddEnumerationField( &ViewportPreferences::m_CameraMode, "m_CameraMode" );
                comp.AddEnumerationField( &ViewportPreferences::m_GeometryMode, "m_GeometryMode" );
                comp.AddField( &ViewportPreferences::m_CameraPrefs, "m_CameraPrefs" );
                comp.AddEnumerationField( &ViewportPreferences::m_ColorMode, "m_ColorMode" );

                comp.AddField( &ViewportPreferences::m_Highlighting, "m_Highlighting" );
                comp.AddField( &ViewportPreferences::m_AxesVisible, "m_AxesVisible" );
                comp.AddField( &ViewportPreferences::m_GridVisible, "m_GridVisible" );
                comp.AddField( &ViewportPreferences::m_BoundsVisible, "m_BoundsVisible" );
                comp.AddField( &ViewportPreferences::m_StatisticsVisible, "m_StatisticsVisible" );
            }
        }; 

        typedef Helium::SmartPtr<ViewportPreferences> ViewportPreferencesPtr; 
    }
}