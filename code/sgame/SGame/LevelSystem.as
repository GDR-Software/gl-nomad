#include "SGame/MapCheckpoint.as"
#include "SGame/MapSpawn.as"
#include "SGame/MapData.as"
#include "SGame/LevelRankData.as"
#include "SGame/LevelInfoData.as"
#include "SGame/LevelStats.as"
#include "SGame/LevelMapData.as"
#include "SGame/InfoSystem/InfoDataManager.as"
#include "Engine/SoundSystem/SoundEffect.as"
#include "SGame/Sprite.as"
#include "SGame/SpriteSheet.as"
#include "SGame/GfxSystem.as"

namespace TheNomad::SGame {
    enum LevelRank {
		RankS,
		RankA,
		RankB,
		RankC,
		RankD,
		RankF,
		RankWereUBotting,
		
		NumRanks
	};

    enum GameState {
		Inactive,
		InLevel,
		LevelFinish,
		EndOfLevel,
		StatsMenu,
		DeathMenu,
	};

    class LevelSystem : TheNomad::GameSystem::GameObject {
		LevelSystem() {
		}

		private void DrawDeathScreen() const {
			ImGui::PushStyleColor( ImGuiCol::WindowBg, vec4( 0.10f, 0.10f, 0.10f, 0.0f ) );

			ImGui::Begin( "##DeathScreen", null, ImGui::MakeWindowFlags( ImGuiWindowFlags::NoResize | ImGuiWindowFlags::NoMove
				| ImGuiWindowFlags::NoCollapse | ImGuiWindowFlags::NoTitleBar | ImGuiWindowFlags::NoScrollbar ) );
			
			ImGui::SetWindowPos( vec2( 0.0f, 280.0f * TheNomad::GameSystem::UIScale ) );
			ImGui::SetWindowSize( vec2( TheNomad::GameSystem::GPUConfig.screenWidth, 260.0f * TheNomad::GameSystem::UIScale ) );

			ImGui::SetWindowFontScale( 5.75f * TheNomad::GameSystem::UIScale );
			ImGui::PushStyleColor( ImGuiCol::Text, colorRed );
			ImGui::SameLine( 286.0f * TheNomad::GameSystem::UIScale );
			ImGui::Text( "YOU DIED" );
			ImGui::PopStyleColor( 1 );
			ImGui::SetWindowFontScale( 1.0f );

			ImGui::SetCursorScreenPos( vec2( 400.0f * TheNomad::GameSystem::UIScale, 440.0f * TheNomad::GameSystem::UIScale ) );
			if ( ImGui::Button( "TRY AGAIN" ) ) {
				// TODO: hellbreaker
				TheNomad::GameSystem::RespawnPlayer();
			}

			ImGui::End();

			ImGui::PopStyleColor( 1 );
		}

		const string& GetName() const {
			return "LevelSystem";
		}
		
		void OnInit() {
			string levelName;
			string mapname;
			string music;
			array<json@> levelInfos;

			ConsolePrint( "Loading level infos...\n" );

			for ( uint i = 0; i < sgame_ModList.Count(); i++ ) {
				LoadLevelsFromFile( sgame_ModList[i], @levelInfos );
			}

			@m_Current = null;

			ConsolePrint( formatUInt( NumLevels() ) + " levels parsed.\n" );

			for ( uint i = 0; i < NumLevels(); i++ ) {
				json@ info = @levelInfos[ i ];
				if ( !info.get( "Name", levelName ) ) {
					ConsoleWarning( "invalid level info, Name variable is missing.\n" );
					continue;
				}

				LevelInfoData@ data = LevelInfoData( levelName );

				ConsolePrint( "Loaded level '" + data.m_Name + "'...\n" );
				
				data.m_MapHandles.Reserve( uint( TheNomad::GameSystem::GameDifficulty::NumDifficulties ) );
				for ( uint j = 0; j < TheNomad::GameSystem::GameDifficulty::NumDifficulties; j++ ) {
					if ( !info.get( "MapNameDifficulty_" + j, mapname ) ) {
						ConsoleWarning( "invalid level map info for \"" + levelName
							+ "\", missing value for 'MapNameDifficulty_" + j + "', skipping.\n" );
						continue;
					}

					LevelMapData MapData = LevelMapData( mapname );
					MapData.mapHandle = TheNomad::GameSystem::LoadMap( mapname );
					MapData.difficulty = TheNomad::GameSystem::GameDifficulty( j );

					if ( MapData.mapHandle == FS_INVALID_HANDLE ) {
						ConsoleWarning( "failed to load map '" + mapname + "' for level '" + data.m_Name + "'\n" );
						continue;
					}
					data.m_MapHandles.Add( MapData );
				}

				if ( info.get( "SoundTrack_AmbientStealth", music ) ) {
					data.m_hAmbientTheme = TheNomad::Engine::SoundSystem::RegisterTrack( music );
					if ( data.m_hAmbientTheme == -1 ) {
						ConsoleWarning( "SoundTrack_AmbientStealth for LevelData \"" + data.m_Name + "\" not found.\n" );
					}
				}
				if ( info.get( "SoundTrack_Combat", music ) ) {
					data.m_hCombatTheme = TheNomad::Engine::SoundSystem::RegisterTrack( music );
					if ( data.m_hCombatTheme == -1 ) {
						ConsoleWarning( "SoundTrack_Combat for LevelData \"" + data.m_Name + "\" not found.\n" );
					}
				}

				info.get( "BeginLevelScript", data.m_StartLevelScript );
				info.get( "EndLevelScript", data.m_EndLevelScript );

				data.m_nIndex = i;

				json@ rankS = @info[ "RankS" ];
				json@ rankA = @info[ "RankA" ];
				json@ rankB = @info[ "RankB" ];
				json@ rankC = @info[ "RankC" ];
				json@ rankD = @info[ "RankD" ];
				json@ rankF = @info[ "RankF" ];
				json@ rankU = @info[ "RankU" ];

				//
				// all rank infos must be present
				//
				if ( @rankS is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankS'.\n" );
					continue;
				}
				if ( @rankA is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankA'.\n" );
					continue;
				}
				if ( @rankB is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankB'.\n" );
					continue;
				}
				if ( @rankC is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankC'.\n" );
					continue;
				}
				if ( @rankD is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankD'.\n" );
					continue;
				}
				if ( @rankF is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankF'.\n" );
					continue;
				}
				if ( @rankU is null ) {
					ConsoleWarning( "invalid level info file, missing object 'RankU'.\n" );
					continue;
				}

				data.m_RankS.rank = LevelRank::RankS;
				if ( !LoadLevelRankData( "RankS", @data.m_RankS, rankS ) ) {
					continue;
				}
				
				data.m_RankA.rank = LevelRank::RankA;
				if ( !LoadLevelRankData( "RankA", @data.m_RankA, rankA ) ) {
					continue;
				}

				data.m_RankB.rank = LevelRank::RankB;
				if ( !LoadLevelRankData( "RankB", @data.m_RankB, rankB ) ) {
					continue;
				}
				
				data.m_RankC.rank = LevelRank::RankC;
				if ( !LoadLevelRankData( "RankC", @data.m_RankC, rankC ) ) {
					continue;
				}
				
				data.m_RankD.rank = LevelRank::RankD;
				if ( !LoadLevelRankData( "RankD", @data.m_RankD, rankD ) ) {
					continue;
				}
				
				data.m_RankF.rank = LevelRank::RankF;
				if ( !LoadLevelRankData( "RankF", @data.m_RankF, rankF ) ) {
					continue;
				}
				
				data.m_RankU.rank = LevelRank::RankWereUBotting;
				if ( !LoadLevelRankData( "RankU", @data.m_RankU, rankU ) ) {
					continue;
				}

				m_LevelInfoDatas.Add( @data );
			}
		}
		void OnShutdown() {
			m_LevelInfoDatas.Clear();
		}
		void OnRenderScene() {
			if ( GlobalState == GameState::StatsMenu ) {
				m_RankData.Draw( true, m_nLevelTimer );
			}
			if ( EntityManager.GetActivePlayer().CheckFlags( EntityFlags::Dead ) ) {
				DrawDeathScreen();
			}
		}
		void OnRunTic() {
			m_RankData.Draw( false, m_nLevelTimer );

			EntityObject@ activeEnts = EntityManager.GetActiveEnts();
			EntityObject@ ent = activeEnts.m_Next;

			int hTrack = m_LevelInfoDatas[ m_nIndex ].m_hAmbientTheme;
			for ( ; ent !is activeEnts; @ent = ent.m_Next ) {
				if ( ent.GetType() == TheNomad::GameSystem::EntityType::Mob ) {
					if ( cast<MobObject@>( ent ).GetTarget() !is null ) {
						hTrack = m_LevelInfoDatas[ m_nIndex ].m_hCombatTheme;
					}
				}
			}

			if ( hTrack != m_hTrack ) {
				TheNomad::Engine::SoundSystem::StopSfx( m_hTrack );
				TheNomad::Engine::SoundSystem::PlaySfx( hTrack );
				m_hTrack = hTrack;
			}
		}
		void OnPlayerDeath( int ) {
		}
		void OnCheckpointPassed( uint ) {
		}
		void OnLevelEnd() {
			if ( m_Current.m_EndLevelScript.Length() != 0 ) {
				string script;
				if ( TheNomad::Engine::FileSystem::LoadFile( m_Current.m_EndLevelScript, script ) == 0 ) {
					ConsoleWarning( "Error loading EndOfLevel script\n" );
				} else {
					ConsolePrint( "Executing \"" + script + "\"\n" );
					TheNomad::Engine::CmdExecuteCommand( script );
				}
			}

			MapCheckpoints.Clear();
			MapSpawns.Clear();
			MapSecrets.Clear();

			for ( uint i = 0; i < MapTileData.Count(); ++i ) {
				MapTileData[i].Clear();
			}
			MapTileData.Clear();
		}

		private void SaveLevelStats() const {
			// save individual stats for each level
			for ( uint i = 0; i < m_LevelInfoDatas.Count(); i++ ) {
				// each section is a different .prt file, so the stats will remain per save-slot unless overwritten
				// hopefully this doesn't lead to other mods overwriting to data if two levels have the same name,
				// but that is a rare case and possibly a user/modder error
				TheNomad::GameSystem::SaveSystem::SaveSection save( m_LevelInfoDatas[i].m_Name + "_RankData" );
				
				// we don't use NumDifficulties because a map doesn't have to be registered with all difficulties
				save.SaveUInt( "MapCount", m_LevelInfoDatas[i].m_MapHandles.Count() );
				for ( uint a = 0; a < m_LevelInfoDatas[i].m_MapHandles.Count(); a++ ) {
					LevelStats@ stats = m_LevelInfoDatas[i].m_MapHandles[a].highStats;
					
					//
					// save the ranks, then the actual numbers (for the perfectionists)
					//
					
					save.SaveUInt( "GameDifficulty", uint( m_LevelInfoDatas[i].m_MapHandles[a].difficulty ) );
					
					save.SaveUInt( "Rank", uint( stats.total_Rank ) );
					save.SaveUInt( "TimeRank", uint( stats.time_Rank ) );
					save.SaveUInt( "KillsRank", uint( stats.kills_Rank ) );
					save.SaveUInt( "StyleRank", uint( stats.style_Rank ) );
					save.SaveUInt( "DeathsRank", uint( stats.deaths_Rank ) );
					
					save.SaveBool( "CleanRun", stats.isClean );
					save.SaveUInt( "Time", stats.m_TimeMilliseconds );
					
					save.SaveUInt( "NumKills", stats.numKills );
					save.SaveUInt( "StylePoints", stats.stylePoints );
					save.SaveUInt( "CollateralScore", stats.collateralScore );
					save.SaveUInt( "NumDeaths", stats.numDeaths );
				}
			}
		}
		private bool LoadLevelStats( const array<string>& in names ) {
			for ( uint i = 0; i < m_LevelInfoDatas.Count(); i++ ) {
				LevelInfoData@ data = m_LevelInfoDatas[i];
				ConsolePrint( "Loading level stats for " + names[i] + "\n" );
				TheNomad::GameSystem::SaveSystem::LoadSection section( data.m_Name + "_RankData" );
				if ( !section.Found() ) {
					GameError( "LevelSystem::OnLoad: save file corruption, section '" + names[i] + "_RankData' not found!" );
				}
				
				// there might be some levels in the save that just aren't present with the mods loaded
				data.m_nIndex = i;
				
				const uint mapCount = section.LoadUInt( "MapCount" );
				data.m_MapHandles.Resize( mapCount );
				for ( uint a = 0; a < mapCount; a++ ) {
					LevelStats@ stats = data.m_MapHandles[a].highStats;
					
					data.m_MapHandles[a].difficulty = TheNomad::GameSystem::GameDifficulty( section.LoadUInt( "GameDifficulty" ) );
					
					stats.total_Rank = LevelRank( section.LoadUInt( "Rank" ) );
					stats.time_Rank = LevelRank( section.LoadUInt( "TimeRank" ) );
					stats.kills_Rank = LevelRank( section.LoadUInt( "KillsRank" ) );
					stats.style_Rank = LevelRank( section.LoadUInt( "StyleRank" ) );
					stats.deaths_Rank = LevelRank( section.LoadUInt( "DeathsRank" ) );

					stats.isClean = section.LoadBool( "CleanRun" );
					stats.m_TimeMilliseconds = section.LoadUInt( "Time" );
					stats.m_TimeSeconds = stats.m_TimeMilliseconds / 1000;
					stats.m_TimeMinutes = stats.m_TimeMilliseconds / 60000;
					
					stats.numKills = section.LoadUInt( "NumKills" );
					stats.stylePoints = section.LoadUInt( "StylePoints" );
					stats.collateralScore = section.LoadUInt( "CollateralScore" );
					stats.numDeaths = section.LoadUInt( "NumDeaths" );
				}
			}
			return true;
		}

		void LoadMap() {
			// get the level index
			m_nIndex = TheNomad::Engine::CvarVariableInteger( "g_levelIndex" );
			const int difficulty = TheNomad::Engine::CvarVariableInteger( "sgame_Difficulty" );

			DebugPrint( "Initializing level at " + m_nIndex + ", difficulty set to \"" + SP_DIFF_STRINGS[ difficulty ] + "\".\n" );
			
			ConsolePrint( "Loading level \"" + m_LevelInfoDatas[ m_nIndex ].m_MapHandles[ difficulty ].m_Name + "\"...\n" );
			
			@m_Current = @m_LevelInfoDatas[ m_nIndex ];
			LoadMapData( m_LevelInfoDatas[ m_nIndex ].m_MapHandles[ difficulty ].mapHandle );

			switch ( TheNomad::GameSystem::GameDifficulty( difficulty ) ) {
			case TheNomad::GameSystem::GameDifficulty::Easy: // Recruit
				m_nDifficultyScale = 0.75f;
				break;
			case TheNomad::GameSystem::GameDifficulty::Normal: // Mercenary
				m_nDifficultyScale = 1.0f;
				break;
			case TheNomad::GameSystem::GameDifficulty::Hard: // Nomad
				m_nDifficultyScale = 1.90f;
				break;
			case TheNomad::GameSystem::GameDifficulty::VeryHard: // The Black Death
				m_nDifficultyScale = 2.5f;
				break;
			case TheNomad::GameSystem::GameDifficulty::Insane: // Insane
				m_nDifficultyScale = 5.0f; // ... ;)
				break;
			};
			
			// check if there's a player spawn at index 0, as that is required for all levels.
			// Even if it's a cinematic where the camera will not be on the player, they must
			// always be active and alive for the level to actually be running
			//
			// in the case we're in a split-screen co-op situation, then all the players after the
			// first will be spawned around them
			if ( !TheNomad::GameSystem::IsLoadGameActive ) {
				if ( MapSpawns.Count() < 1 || MapCheckpoints.Count() < 1 ) {
					ForcePlayerSpawn();
				} else {
					const MapSpawn@ spawn = MapSpawns[ 0 ];
					if ( spawn.m_nEntityType != TheNomad::GameSystem::EntityType::Playr ) {
						ForcePlayerSpawn();
					}
				}
			}

			if ( m_Current.m_StartLevelScript.Length() != 0 ) {
				string script;
				if ( TheNomad::Engine::FileSystem::LoadFile( m_Current.m_StartLevelScript, script ) == 0 ) {
					ConsoleWarning( "Error loading StartOfLevel script\n" );
				} else {
					TheNomad::Engine::CmdExecuteCommand( script );
				}
			}
		}

		void OnSave() const {
			const LevelStats@ stats;
			
			DebugPrint( "Archiving level stats...\n" );
			
			{
				TheNomad::GameSystem::SaveSystem::SaveSection section( GetName() );
				section.SaveUInt( "NumLevels", m_LevelInfoDatas.Count() );
				for ( uint i = 0; i < m_LevelInfoDatas.Count(); i++ ) {
					section.SaveString( "LevelNames" + i, m_LevelInfoDatas[i].m_Name );
				}

				section.SaveUInt( "Time", m_nLevelTimer );
				section.SaveInt( "MapIndex", m_nIndex );
				section.SaveInt( "Difficulty", sgame_Difficulty.GetInt() );

				// save checkpoint data
				if ( m_CurrentCheckpoint < MapCheckpoints.Count() - 1 ) {
					section.SaveUInt( "CurrentCheckpoint", m_CurrentCheckpoint );
				}
				for ( uint i = 0; i < m_CurrentCheckpoint; i++ ) {
					section.SaveUInt( "CheckpointTime_" + i, MapCheckpoints[i].m_nTime );
				}
			}

			SaveLevelStats();
		}

		void OnLoad() {
			LevelInfoData@ data;
			array<string> names;

			TheNomad::GameSystem::SaveSystem::LoadSection load( GetName() );
			DebugPrint( "Loading level stats...\n" );
			if ( !load.Found() ) {
				GameError( "LevelSystem::OnLoad: save file corruption, section '" + GetName() + "' not found!" );
			}

			const uint numLevels = load.LoadUInt( "NumLevels" );
			ConsolePrint( "Got " + numLevels + " levels\n" );
			m_LevelInfoDatas.Resize( numLevels );
			names.Resize( numLevels );
			for ( uint i = 0; i < m_LevelInfoDatas.Count(); i++ ) {
				load.LoadString( "LevelNames" + i, names[i] );
			}

			m_nLevelTimer = load.LoadUInt( "Time" );
			TheNomad::Engine::CvarSet( "g_levelIndex", formatUInt( load.LoadInt( "MapIndex" ) ) );
			TheNomad::Engine::CvarSet( "sgame_Difficulty", formatUInt( load.LoadInt( "Difficulty" ) ) );
			m_CurrentCheckpoint = load.LoadUInt( "CurrentCheckpoint" );
			for ( uint i = 0; i < m_CurrentCheckpoint; i++ ) {
				MapCheckpoints[ i ].m_nTime = load.LoadUInt( "CheckpointTime_" + i );
			}

			if ( !LoadLevelStats( names ) ) {
				GameError( "Couldn't load level scores from section '" + GetName() + "'" );
			}

			LoadMap();
		}
		void OnLevelStart() {
			m_RankData = LevelStats();
			m_CurrentCheckpoint = 0;
			m_nIndex = 0;
			m_nLevelTimer = TheNomad::GameSystem::GameTic;

			if ( !TheNomad::GameSystem::IsLoadGameActive ) {
				LoadMap();
			}

			m_hTrack = m_LevelInfoDatas[ m_nIndex ].m_hAmbientTheme;
			TheNomad::Engine::SoundSystem::PlaySfx( m_hTrack );
		}
		
		private void ForcePlayerSpawn() {
			MapSpawn spawn( uvec3( 0, 0, 0 ), 0, TheNomad::GameSystem::EntityType::Playr );

			ConsoleWarning( "LevelSystem::OnLevelStart: the first spawn in a level must always be the player!\n" );
			ConsolePrint( "Forcing player spawn...\n" );
			MapSpawns.InsertAt( 0, spawn );
			if ( MapCheckpoints.Count() == 0 ) {
				ConsoleWarning( "LevelSystem::OnLevelStart: at least one checkpoint is required per level\n" );
				ConsolePrint( "Forcing mandatory checkpoint...\n" );

				MapCheckpoint cp = MapCheckpoint( uvec3( 0, 0, 0 ), uvec2( 0, 0 ), 0 );
				MapCheckpoints.InsertAt( 0, cp );
			}
			MapCheckpoints[0].m_Spawns.Add( spawn );
		}
		
		//
		// AddLevelData: for manual level infos
		//
		void AddLevelData( LevelInfoData@ levelData ) {
			m_LevelInfoDatas.Add( levelData );
		}
		
		const LevelInfoData@ GetLevelDataByIndex( uint nIndex ) const {
			return m_LevelInfoDatas[ nIndex ];
		}
		LevelInfoData@ GetLevelDataByIndex( uint nIndex ) {
			return m_LevelInfoDatas[ nIndex ];
		}
		
		const LevelInfoData@ GetLevelInfoByMapName( const string& in mapname ) const {
			for ( uint i = 0; i < m_LevelInfoDatas.Count(); i++ ) {
				for ( uint a = 0; a < m_LevelInfoDatas[i].m_MapHandles.Count(); a++ ) {
					if ( Util::StrICmp( m_LevelInfoDatas[i].m_MapHandles[a].m_Name, mapname ) == 0 ) {
						return m_LevelInfoDatas[i];
					}
				}
			}
			return null;
		}

		private array<json@>@ LoadJSonFile( const string& in modName ) {
			string path;
			array<json@> values;
			json@ data;
			
			path = "modules/" + modName + "/DataScripts/levels.json";
			
			@data = json();
			if ( !data.ParseFile( path ) ) {
				ConsoleWarning( "failed to load level info file '" + path + "', skipping.\n" );
				return null;
			}

			if ( !data.get( "LevelInfo", values ) ) {
				ConsoleWarning( "level info file found, but no level infos found.\n" );
				return null;
			}
			
			return @values;
		}

		private void LoadLevelsFromFile( const string& in modName, array<json@>@ levelInfos ) {
			array<json@>@ levels;

			@levels = @LoadJSonFile( modName );
			if ( @levels is null ) {
				return;
			}

			ConsolePrint( "Got " + levels.Count() + " level infos from \"" + modName + "\"\n" );
			m_nLevels += levels.Count();
			levelInfos.Reserve( levels.Count() );
			for ( uint i = 0; i < levels.Count(); i++ ) {
				levelInfos.Add( levels[i] );
			}
		}

		bool LoadLevelRankData( const string& in rankName, LevelRankData@ data, json@ src ) {
			json@ minTime;

			if ( !src.get( "MinKills", data.minKills ) ) {
				ConsoleWarning( "invalid level info object '" + rankName + "', no variable 'MinKills'.\n" );
				return false;
			}
			data.minKills = uint( src[ "MinKills" ] );

			if ( !src.get( "MinStyle", data.minStyle ) ) {
				ConsoleWarning( "invalid level info object '" + rankName + "', no variable 'MinStyle'.\n" );
				return false;
			}
			data.minStyle = uint( src[ "MinStyle" ] );

			if ( !src.get( "MaxDeaths", data.maxDeaths ) ) {
				ConsoleWarning( "invalid level info object '" + rankName + "', no variable 'MaxDeaths'.\n" );
				return false;
			}
			data.maxDeaths = uint( src[ "MaxDeaths" ] );

			if ( !src.get( "MaxCollateral", data.maxCollateral ) ) {
				ConsoleWarning( "invalid level info object '" + rankName + "', no variable 'MaxCollateral'.\n" );
				return false;
			}
			data.maxCollateral = uint( src[ "MaxCollateral" ] );

			if ( !src.get( "RequiresClean", data.requiresClean ) ) {
				ConsoleWarning( "invalid level info object '" + rankName + "', no variable 'RequiresClean'.\n" );
				return false;
			}
			data.requiresClean = bool( src[ "RequiresClean" ] );

			if ( !src.get( "MinTime", data.minTime ) ) {
				ConsoleWarning( "invalid level info object '" + rankName + "', no variable 'MinTime'.\n" );
				return false;
			}
			data.minTime = uint( src[ "MinTime" ] );

			return true;
		}

		private void CalcTotalLevelTime() {
			uint total = 0;

			for ( uint i = 0; i < MapCheckpoints.Count(); i++ ) {
				total += MapCheckpoints[i].m_nTime;
			}

			m_nLevelTimer = total;
		}
		void CalcLevelStats() {
			CalcTotalLevelTime();
			m_RankData.CalcTotalLevelStats();
		}

		private MapCheckpoint@ PlayerPassedCheckpoint( uint& out nCheckpointIndex ) {
			const vec3 origin = EntityManager.GetActivePlayer().GetOrigin();
			array<MapCheckpoint>@ checkpoints = MapCheckpoints;
			const uvec3 pos = uvec3( uint( origin.x ), uint( origin.y ), uint( origin.z ) );
			MapCheckpoint@ cp = null;

			for ( uint i = 0; i < checkpoints.Count(); ++i ) {
				@cp = checkpoints[i];
				if ( cp.m_Origin == pos && !cp.m_bPassed ) {
					nCheckpointIndex = i;
					return cp;
				}
			}
			return null;
		}

		void Pause() {
			if ( m_bPaused ) {
				return;
			}
			m_bPaused = true;
			m_nPauseTimer = TheNomad::GameSystem::GameTic;
		}
		void Resume() {
			if ( !m_bPaused ) {
				return;
			}
			m_bPaused = false;

			/*
			* ===========] pause [================================
			* level time | delta | level time = level time + delta
			*/
			const uint delta = TheNomad::GameSystem::GameTic - m_nPauseTimer;
			m_nLevelTimer += delta;
		}

		float GetDifficultyScale() const {
			return m_nDifficultyScale;
		}
		
		LevelStats& GetStats() {
			return m_RankData;
		}
		
		uint NumLevels() const {
			return m_nLevels;
		}
		uint LoadedIndex() const {
			return m_nIndex;
		}
		uint GetCheckpointIndex() const {
			return m_CurrentCheckpoint;
		}
		uint GetLevelTimer() const {
			return m_nLevelTimer;
		}

		const LevelInfoData@ GetCurrentData() const {
			return m_Current;
		}
		LevelInfoData@ GetCurrentData() {
			return m_Current;
		}
		const string GetLevelName() const {
			return m_Current !is null ? m_Current.m_Name : "N/A";
		}

		void CheckNewGamePlus() {
			if ( m_CurrentCheckpoint < MapCheckpoints.Count() - 1 ) {
				ConsolePrint( "NewGamePlus not active.\n" );
				return;
			}
			ConsolePrint( "NewGamePlus active.\n" );
			m_CurrentCheckpoint = 0;
		}
		
		private uint m_nEndTime = 0;
		private uint m_nPauseTimer = 0;
		private int m_nLevelTimer = 0;
		private uint m_CurrentCheckpoint = 0;
		private array<LevelInfoData@> m_LevelInfoDatas;
		private uint m_nIndex = 0;
		private uint m_nLevels = 0;
		private int m_hTrack = FS_INVALID_HANDLE;
		private bool m_bPaused = false;
		
		private bool m_bNewCheckpoint = true;
		private TheNomad::Engine::SoundSystem::SoundEffect m_PassedCheckpointSfx;

		private float m_nDifficultyScale = 1.0f;
		
		private LevelStats m_RankData = LevelStats();
		private LevelInfoData@ m_Current = null;
	};

	uint GetMapLevel( float elevation ) {
//		return uint( floor( elevation ) );
		return 0;
	}

	array<string> sgame_ModList;

	GameState GlobalState;

	LevelSystem@ LevelManager = null;
	
	string[] SP_DIFF_STRINGS( TheNomad::GameSystem::GameDifficulty::NumDifficulties );
	string[] sgame_RankStrings( TheNomad::SGame::LevelRank::NumRanks );
	vec4[] sgame_RankStringColors( TheNomad::SGame::LevelRank::NumRanks );
};