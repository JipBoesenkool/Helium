#include "Precompile.h"
#include "EntityPanel.h"
#include "EntityAssetSet.h"

#include "Application.h"

#include "Application/Inspect/Widgets/Text Controls/Value.h"
#include "Application/Inspect/Widgets/Button Controls/InspectButton.h"

#include "Pipeline/Asset/AssetInit.h"
#include "Foundation/Component/ComponentHandle.h"
#include "Pipeline/Asset/AssetClass.h"
#include "Foundation/Log.h"
#include "Platform/Process.h"
#include "Task/Export.h"
#include "Volume.h"

#include "Light.h"

#include "Scene.h"
#include "SceneManager.h"
#include "SceneEditor.h"

#include "Editor/Editor.h"

using namespace Reflect;
using namespace Asset;
using namespace Luna;

template <class T>
bool SelectionHasAttribute(const OS_SelectableDumbPtr& selection)
{
    OS_SelectableDumbPtr::Iterator itr = selection.Begin();
    OS_SelectableDumbPtr::Iterator end = selection.End();
    for ( ; itr != end; ++itr )
    {
        Luna::SceneNode* node = Reflect::ObjectCast< Luna::SceneNode >( *itr );

        if (!node)
        {
            return false;
        }

        Content::SceneNode* packageNode = node->GetPackage<Content::SceneNode>();

        if (!packageNode)
        {
            return false;
        }

        Component::ComponentViewer<T> attr ( packageNode );

        if (!attr.Valid())
        {
            return false;
        }
    }

    return true;
}

template <class T>
bool SelectionHasSameAttribute(const OS_SelectableDumbPtr& selection, Component::ComponentViewer< T >& attribute)
{
    OS_SelectableDumbPtr::Iterator itr = selection.Begin();
    OS_SelectableDumbPtr::Iterator end = selection.End();
    for ( ; itr != end; ++itr )
    {
        Luna::SceneNode* node = Reflect::ObjectCast< Luna::SceneNode >( *itr );

        if (!node)
        {
            return false;
        }

        Content::SceneNode* packageNode = node->GetPackage<Content::SceneNode>();

        if (!packageNode)
        {
            return false;
        }

        if ( itr == selection.Begin() )
        {
            attribute.View( packageNode );
            if ( !attribute.Valid() )
            {
                return false;
            }
        }
        else
        {
            Component::ComponentViewer< T > nextAttribute = Component::ComponentViewer< T >( packageNode );
            if ( !nextAttribute.Valid() || ( attribute.operator->() != nextAttribute.operator->() ) )
            {
                return false;
            }
        }
    }

    return true;
}

EntityPanel::EntityPanel(Enumerator* enumerator, const OS_SelectableDumbPtr& selection)
: InstancePanel (enumerator, selection)
, m_LightingPanel ( NULL )
, m_DrawDistance ( NULL )
, m_UpdateDistance ( NULL )
, m_ShaderGroup ( NULL )
, m_Attenuation( NULL )
, m_FarShadowFadeout( NULL )
, m_CastsBakedShadows( NULL )
, m_DoBakedShadowCollisions( NULL )
, m_BakedShadowAABBExt ( NULL )
, m_BakedShadowMergeGroups( NULL )
, m_HighResShadowMap( NULL )
, m_SegmentEnabler ( NULL )
, m_ShaderGroupEnabler ( NULL )
, m_FarShadowFadeoutEnabler( NULL )
, m_CastsBakedShadowsEnabler( NULL )
, m_DoBakedShadowCollisionsEnabler( NULL )
, m_BakedShadowAABBExtEnabler( NULL )
, m_BakedShadowMergeGroupsEnabler( NULL )
, m_HighResShadowMapEnabler( NULL )
{
    m_Expanded = true;
    m_Text = TXT( "Entity" );

    OS_SelectableDumbPtr::Iterator itr = m_Selection.Begin();
    OS_SelectableDumbPtr::Iterator end = m_Selection.End();
    for ( ; itr != end; ++itr )
    {
        Luna::Entity* entity = Reflect::AssertCast< Luna::Entity >( *itr );
        if ( entity && entity->GetClassSet() )
        {
            entity->GetClassSet()->AddClassLoadedListener( EntityAssetSetChangeSignature::Delegate ( this, &EntityPanel::EntityAssetReloaded ) );
            m_Entities.push_back( entity );
        }
    }

    m_SegmentEnabler = new ControlEnabler ();
    m_ShaderGroupEnabler = new ControlEnabler ();
    m_FarShadowFadeoutEnabler = new ControlEnabler ();
    m_CastsBakedShadowsEnabler = new ControlEnabler ();
    m_DoBakedShadowCollisionsEnabler = new ControlEnabler ();
    m_BakedShadowAABBExtEnabler = new ControlEnabler ();
    m_BakedShadowMergeGroupsEnabler = new ControlEnabler ();
    m_HighResShadowMapEnabler = new ControlEnabler ();
}

EntityPanel::~EntityPanel()
{
    V_EntitySmartPtr::const_iterator itr = m_Entities.begin();
    V_EntitySmartPtr::const_iterator end = m_Entities.end();
    for ( ; itr != end; ++itr )
    {
        Luna::Entity* entity = *itr;
        if ( entity->GetClassSet() )
        {
            entity->GetClassSet()->RemoveClassLoadedListener( EntityAssetSetChangeSignature::Delegate ( this, &EntityPanel::EntityAssetReloaded ) );
        }
    }

    delete m_SegmentEnabler;
    delete m_ShaderGroupEnabler;
    delete m_FarShadowFadeoutEnabler;
    delete m_CastsBakedShadowsEnabler;
    delete m_DoBakedShadowCollisionsEnabler;
    delete m_BakedShadowAABBExtEnabler;
    delete m_BakedShadowMergeGroupsEnabler;
    delete m_HighResShadowMapEnabler;
}

void EntityPanel::CreateAssetType()
{
    m_Enumerator->PushContainer();
    {
        m_Enumerator->AddLabel( TXT( "Engine Type" ) );
        Inspect::Value* textBox = m_Enumerator->AddValue<Luna::Entity, tstring>( m_Selection, &Luna::Entity::GetAssetTypeName, &Luna::Entity::SetAssetTypeName );
        textBox->SetReadOnly( true );
    }
    m_Enumerator->Pop();
}

void EntityPanel::CreateClassPath()
{
    m_Enumerator->PushContainer();
    {
        m_Enumerator->AddLabel( TXT( "Class Path" ) );

        m_TextBox = m_Enumerator->AddValue<Luna::Entity, tstring>( m_Selection, &Luna::Entity::GetEntityAssetPath, &Luna::Entity::SetEntityAssetPath );
        m_TextBox->AddBoundDataChangingListener( Inspect::ChangingSignature::Delegate ( this, &EntityPanel::OnEntityAssetChanging ) );
        m_TextBox->AddBoundDataChangedListener( Inspect::ChangedSignature::Delegate ( this, &EntityPanel::OnEntityAssetChanged ) );

        Inspect::FileDialogButton* fileButton = m_Enumerator->AddFileDialogButton<Luna::Entity, tstring>( m_Selection, &Luna::Entity::GetEntityAssetPath, &Luna::Entity::SetEntityAssetPath );
        fileButton->AddBoundDataChangingListener( Inspect::ChangingSignature::Delegate ( this, &EntityPanel::OnEntityAssetChanging ) );
        fileButton->AddBoundDataChangedListener( Inspect::ChangedSignature::Delegate ( this, &EntityPanel::OnEntityAssetChanged ) );

        Inspect::FileBrowserButton* browserButton = m_Enumerator->AddFileBrowserButton<Luna::Entity, tstring>( m_Selection, &Luna::Entity::GetEntityAssetPath, &Luna::Entity::SetEntityAssetPath );
        browserButton->AddBoundDataChangingListener( Inspect::ChangingSignature::Delegate ( this, &EntityPanel::OnEntityAssetChanging ) );
        browserButton->AddBoundDataChangedListener( Inspect::ChangedSignature::Delegate ( this, &EntityPanel::OnEntityAssetChanged ) );

        tstring filter;
        Reflect::GetClass<Asset::Entity>()->GetProperty( Asset::AssetProperties::FileFilter, filter );

        if ( !filter.empty() )
        {
            fileButton->SetFilter( filter );
            browserButton->SetFilter( filter );
        }
        else
        {
            // There's a problem, better disable the button
            fileButton->SetEnabled( false );
            browserButton->SetEnabled( false );
        }

        Inspect::FilteredDropTarget* filteredDropTarget = new Inspect::FilteredDropTarget( filter );
        filteredDropTarget->AddDroppedListener( Inspect::FilteredDropTargetSignature::Delegate( this, &EntityPanel::OnEntityAssetDrop ) );
        m_TextBox->SetDropTarget( filteredDropTarget );
    }
    m_Enumerator->Pop();

}

void EntityPanel::CreateClassActions()
{
    m_Enumerator->PushContainer();
    {
        m_Enumerator->AddLabel( TXT( "Class Actions" ) );

        Inspect::Action* refreshButton = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnEntityAssetRefresh ) );
        refreshButton->SetIcon( TXT( "actions/view-refresh.png" ) );
        refreshButton->SetToolTip( TXT( "Refresh" ) );

        bool singular = m_Selection.Size() == 1;

        Inspect::Action* lunaButton = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnEntityAssetEditAsset ) );
        lunaButton->SetIcon( TXT( "asset_editor.png" ) );
        lunaButton->SetToolTip( TXT( "Edit this entity class in Luna's Asset Editor" ) );

        Inspect::Action* mayaButton = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnEntityAssetEditArt ) );
        mayaButton->SetIcon( TXT( "maya.png" ) );
        mayaButton->SetEnabled( singular );
        mayaButton->SetToolTip( TXT( "Edit this entity class's art in Maya" ) );

        Inspect::Action* historyButton = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnEntityAssetRevisionHistory ) );
        historyButton->SetIcon( TXT( "p4.png" ) );
        historyButton->SetToolTip( TXT( "Display revision history for this file in Perforce." ) );
    }
    m_Enumerator->Pop();
}



void EntityPanel::CreateChildImportExport()
{
    m_Enumerator->PushPanel( TXT( "Child Import/Export" ) );
    {
        m_Enumerator->PushContainer();
        {
            Inspect::Action* button1 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnExport< Luna::Transform, Content::Transform > ) );
            button1->SetText( TXT( "Export All" ) );

            Inspect::Action* button2 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnImport< Luna::Transform, Content::Transform > ) );
            button2->SetText( TXT( "Import All" ) );

            Inspect::Action* button3 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnSelectChildren< Luna::Transform > ) );
            button3->SetText( TXT( "Select All" ) );

            Inspect::Action* button4 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnExportToFile< Luna::Transform, Content::Transform > ) );
            button4->SetText( TXT( "Export To File" ) );

            Inspect::Action* button5 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnImportFromFile< Luna::Transform, Content::Transform > ) );
            button5->SetText( TXT( "Import From File" ) );

        }
        m_Enumerator->Pop();

        m_Enumerator->PushContainer();
        {
            Inspect::Action* button1 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnExport< Luna::Entity, Asset::EntityInstance > ) );
            button1->SetText( TXT( "Export Entities" ) );

            Inspect::Action* button2 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnImport< Luna::Entity, Asset::EntityInstance > ) );
            button2->SetText( TXT( "Import Entities" ) );

            Inspect::Action* button3 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnSelectChildren< Luna::Entity > ) );
            button3->SetText( TXT( "Select Entities" ) );
        }
        m_Enumerator->Pop();

        m_Enumerator->PushContainer();
        {
            Inspect::Action* button1 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnExport< Luna::Volume, Content::Volume > ) );
            button1->SetText( TXT( "Export Weather Volumes" ) );

            Inspect::Action* button2 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnImport< Luna::Volume, Content::Volume > ) );
            button2->SetText( TXT( "Import Weather Volumes" ) );

            Inspect::Action* button3 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnSelectChildren< Luna::Volume > ) );
            button3->SetText( TXT( "Select Weather Volumes" ) );
        }
        m_Enumerator->Pop();

        m_Enumerator->PushContainer();
        {
            Inspect::Action* button1 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnExport< Luna::Light, Content::Light > ) );
            button1->SetText( TXT( "Export Lights" ) );

            Inspect::Action* button2 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnImport< Luna::Light, Content::Light > ) );
            button2->SetText( TXT( "Import Lights" ) );

            Inspect::Action* button3 = m_Enumerator->AddAction( Inspect::ActionSignature::Delegate( this, &EntityPanel::OnSelectChildren< Luna::Light > ) );
            button3->SetText( TXT( "Select Lights" ) );
        }
        m_Enumerator->Pop();
    }
    m_Enumerator->Pop();
}

void EntityPanel::CreateShowFlags()
{
    m_Enumerator->PushContainer();
    {
        m_Enumerator->AddLabel( TXT( "Show Pointer" ) );
        m_Enumerator->AddCheckBox<Luna::Entity, bool>( m_Selection, 
            &Luna::Entity::IsPointerVisible, 
            &Luna::Entity::SetPointerVisible, false );
    }
    m_Enumerator->Pop();

    m_Enumerator->PushContainer();
    {
        m_Enumerator->AddLabel( TXT( "Show Bounds" ) );
        m_Enumerator->AddCheckBox<Luna::Entity, bool>( m_Selection, 
            &Luna::Entity::IsBoundsVisible, 
            &Luna::Entity::SetBoundsVisible, false );
    }
    m_Enumerator->Pop();

    m_Enumerator->PushContainer();
    {
        m_Enumerator->AddLabel( TXT( "Show Geometry" ) );
        m_Enumerator->AddCheckBox<Luna::Entity, bool>( m_Selection, 
            &Luna::Entity::IsGeometryVisible, 
            &Luna::Entity::SetGeometryVisible, false );
    }
    m_Enumerator->Pop();
}



void EntityPanel::Create()
{
    CreateApplicationType();
    CreateAssetType(); 

    CreateClassPath(); 
    CreateClassActions(); 

    CreateShowFlags(); 
    CreateAppearanceFlags();

    CreateChildImportExport();

    Inspect::Panel::Create();
}


///////////////////////////////////////////////////////////////////////////////
// Validation function for the controls that change the Entity Class field.
// Returns true if the new value for the Entity Class field can be resolved to
// a file TUID.
//
bool EntityPanel::OnEntityAssetChanging( const Inspect::ChangingArgs& args )
{
    bool result = false;

    tstring newValue;
    Reflect::Serializer::GetValue(args.m_NewValue, newValue);

    Nocturnal::Path path( newValue );
    if ( path.Exists() )
    {
        // Make sure the file has a reflect extension
        std::set< tstring > extensions;
        Reflect::Archive::GetExtensions( extensions );
        if ( extensions.find( path.Extension() ) != extensions.end() )
        {
            result = true;
        }
    }

    if ( !result )
    {
        // Message to the user that the value is not correct.
        wxMessageBox( wxT( "Invalid Entity Class specified!" ), wxT( "Error" ), wxOK | wxCENTER | wxICON_ERROR, GetWindow() );
    }

    return result;
}

void EntityPanel::OnEntityAssetChanged( const Inspect::ChangeArgs& args )
{
}

void EntityPanel::OnEntityAssetRefresh( Inspect::Button* button )
{
    Luna::Scene* scene = NULL;

    // when we refresh, reload the common class set information in case
    // we did something like reexport an art class, while luna is still opened
    std::set< EntityAssetSet* > reloadQueue;     // entities we want to reload

    std::set< tstring > files;

    OS_SelectableDumbPtr::Iterator selectionIter = m_Selection.Begin();
    OS_SelectableDumbPtr::Iterator selectionEnd = m_Selection.End();
    for (; selectionIter != selectionEnd; ++selectionIter )
    {
        Luna::Entity* entity = Reflect::ObjectCast< Luna::Entity >( *selectionIter );

        if ( !scene )
        {
            Luna::SceneNode* node = Reflect::ObjectCast< Luna::SceneNode >( *selectionIter );
            scene = node->GetScene();
        }

        if( entity->GetClassSet( ) )
        {
            reloadQueue.insert( entity->GetClassSet( ) );
        }

        if (entity->IsGeometryVisible())
        {
            for ( int i=0; i<GeometryModes::Count; i++ )
            {
                Luna::Scene* nestedScene = entity->GetNestedScene(entity->GetScene()->GetView()->GetGeometryMode());

                if (nestedScene)
                {
                    if ( files.insert( nestedScene->GetFullPath() ).second )
                    {
                        // Only reload the scene if we haven't already done so during this iteration.
                        nestedScene->Reload();
                    }
                }
            }
        }
    }

    std::set< EntityAssetSet* >::iterator itr = reloadQueue.begin();
    std::set< EntityAssetSet* >::iterator end = reloadQueue.end();

    while( itr != end )
    {
        EntityAssetSet* entClassSet = ObjectCast< EntityAssetSet >( *itr );

        if( entClassSet )
        {
            entClassSet->LoadAssetClass( );
        }

        itr++;
    }

    if (scene)
    {
        scene->Execute(false);
    }
}

void EntityPanel::OnEntityAssetEditAsset( Inspect::Button* button )
{
    std::set< tstring > files;
    OS_SelectableDumbPtr::Iterator selectionIter = m_Selection.Begin();
    OS_SelectableDumbPtr::Iterator selectionEnd = m_Selection.End();
    for ( ; selectionIter != selectionEnd; ++selectionIter )
    {
        Luna::Entity* entity = Reflect::ObjectCast< Luna::Entity >( *selectionIter );
        if ( entity )
        {
            tstring fileToEdit = entity->GetEntityAssetPath();
            if ( !fileToEdit.empty() )
            {
                files.insert( fileToEdit );
            }
        }
    }

    std::set< tstring >::const_iterator fileItr = files.begin();
    std::set< tstring >::const_iterator fileEnd = files.end();
    for ( ; fileItr != fileEnd; ++fileItr )
    {
#pragma TODO( "Open the file for edit" )
        NOC_BREAK();
    }
}

void EntityPanel::OnEntityAssetEditArt( Inspect::Button* button )
{
}

void EntityPanel::OnEntityAssetRevisionHistory( Inspect::Button* button )
{
    std::set< tstring > files;
    SceneEditor* editor = wxGetApp().GetSceneEditor();
    OS_SelectableDumbPtr::Iterator selectionItr = m_Selection.Begin();
    OS_SelectableDumbPtr::Iterator selectionEnd = m_Selection.End();
    for ( ; selectionItr != selectionEnd; ++selectionItr )
    {
        Luna::Entity* entity = Reflect::ObjectCast< Luna::Entity >( *selectionItr );
        tstring path = entity->GetEntityAssetPath();
        if ( !path.empty() )
        {
            files.insert( path );
        }
    }

    std::set< tstring >::const_iterator pathItr = files.begin();
    std::set< tstring >::const_iterator pathEnd = files.end();
    for ( ; pathItr != pathEnd; ++pathItr )
    {
        editor->RevisionHistory( *pathItr );
    }
}

void EntityPanel::OnEntityAssetDrop( const Inspect::FilteredDropTargetArgs& args )
{
    if ( args.m_Paths.size() )
    {
        m_TextBox->WriteData( args.m_Paths[ 0 ] );
    }
}


///////////////////////////////////////////////////////////////////////////////
// Callback for when one of the selected entity class sets is reloaed. 
// 
void EntityPanel::EntityAssetReloaded( const EntityAssetSetChangeArgs& args )
{
    Read();
}

