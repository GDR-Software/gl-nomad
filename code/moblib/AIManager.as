#include "nomadmain/SGame/InfoSystem/InfoDataManager.as"
#include "moblib/MobScript.as"

namespace moblib {
	class AISystem : TheNomad::GameSystem::GameObject {
		AISystem() {
		}
		
		uint GetMobStateOffset( uint nType ) const {
			return uint( TheNomad::SGame::StateNum::ST_MOB_IDLE ) + nType;
		}
		void AllocScript( TheNomad::SGame::MobObject@ mob ) {
			MobScript@ script = cast<MobScript@>( @TheNomad::Util::AllocateExternalScriptClass( "moblib",
				cast<TheNomad::SGame::InfoSystem::MobInfo@>( mob.GetInfo() ).className ) );
			mob.LinkScript( @script );
			m_ScriptFactory.Add( @script );
		}
		
		const string& GetName() const {
			return "AIManager";
		}
		void OnInit() {
		}
		void OnShutdown() {
		}
		void OnLevelStart() {
			dictionary@ mobs = @TheNomad::SGame::InfoSystem::InfoManager.GetMobInfos();
			const array<TheNomad::SGame::InfoSystem::EntityData>@ types = @TheNomad::SGame::InfoSystem::InfoManager.GetMobTypes();
			array<TheNomad::SGame::EntityObject@>@ ents = @TheNomad::SGame::EntityManager.GetEntities();

			ConsolePrint( "Linking mob scripts...\n" );

			for ( uint i = 0; i < ents.Count(); i++ ) {
				if ( ents[i].GetType() != TheNomad::GameSystem::EntityType::Mob ) {
					continue;
				}

				TheNomad::SGame::MobObject@ mob = cast<TheNomad::SGame::MobObject@>( @ents[i] );
				AllocScript( @mob );
			}
		}
		void OnLevelEnd() {
			m_ScriptFactory.Clear();
		}
		void OnSave() const {
		}
		void OnLoad() {
		}
		void OnPlayerDeath( int ) {
		}
		void OnCheckpointPassed( uint ) {
		}
		void OnRunTic() {
		}

		void OnRenderScene() {
		}
		
		private array<MobScript@> m_ScriptFactory;
	};
	
	AISystem@ AIManager = null;
};