#include "SGame/KeyBind.as"
#include "SGame/PlayerSystem/PlayerSystem.as"

namespace TheNomad::SGame {
	const float HEAL_MULT_BASE = 0.001f;

	const uint PF_SLIDING			= 0x00000001;
	const uint PF_CROUCHING			= 0x00000002;
	const uint PF_REFLEX			= 0x00000004;
	const uint PF_DASHING			= 0x00000008;
	const uint PF_BULLET_TIME		= 0x00000010;
	const uint PF_DEMON_RAGE		= 0x00000020;
	const uint PF_INVUL				= 0x00000040;
	const uint PF_USED_MANA			= 0x00000080;
	const uint PF_USING_WEAPON		= 0x00000100;
	const uint PF_USING_WEAPON_ALT	= 0x00000200;
	const uint PF_AFTER_IMAGE		= 0x10000000;
	const uint PF_BACKPEDAL			= 0x20000000;

	const uint LEFT_ARM				= 0;
	const uint RIGHT_ARM			= 1;
	const uint BOTH_ARMS			= 2;

	const uint RAW_REFLEX_TIME		= 8000;
	const uint REFLEX_TIME			= 8000;

	const uint DASH_COOLDOWN		= 200;

	const uint[] sgame_WeaponModeList = {
		uint( InfoSystem::WeaponProperty::IsOneHanded ) | uint( InfoSystem::WeaponProperty::IsBladed ),
		uint( InfoSystem::WeaponProperty::IsOneHanded ) | uint( InfoSystem::WeaponProperty::IsBlunt ),
		uint( InfoSystem::WeaponProperty::IsOneHanded ) | uint( InfoSystem::WeaponProperty::IsFirearm ),

		uint( InfoSystem::WeaponProperty::IsTwoHanded ) | uint( InfoSystem::WeaponProperty::IsBladed ),
		uint( InfoSystem::WeaponProperty::IsTwoHanded ) | uint( InfoSystem::WeaponProperty::IsBlunt ),
		uint( InfoSystem::WeaponProperty::IsTwoHanded ) | uint( InfoSystem::WeaponProperty::IsFirearm )
	};
	
    class PlayrObject : EntityObject {
		PlayrObject() {
		}
		
		/*
		private bool TryOneHanded( HandMode hand, const WeaponObject@ Arm, const WeaponObject@ otherWeapon ) {
			if ( hand != HandMode::Empty ) {
				if ( ( Arm.IsTwoHanded() && Arm.IsOneHanded() ) && @otherWeapon is @Arm ) {
					// being used by both arms, switch to one-handed
					return true;
				}
			}
			return false;
		}
		
		private WeaponObject@ GetEmptyHand() {
			if ( TryOneHanded( m_LeftHandMode, @LeftArm, @RightArm ) ) {
				m_LeftHandMode = HandMode::OneHanded;
				m_RightHandMode = HandMode::Empty;
				@RightArm = @RightArm;
				return @RightArm;
			}
			else if ( TryOneHanded( m_RightHandMode, @RightArm, @LeftArm ) ) {
				m_RightHandMode = HandMode::OneHanded;
				m_LeftHandMode = HandMode::Empty;
				@LeftArm = @LeftArm;
				return @LeftArm;
			}
		}
		*/

		bool opCmp( const PlayrObject& in other ) const {
			return this is @other;
		}

		//
		// PlayrObject::SwitchWeaponWielding: swaps a weapon in one hand to the other hand
		//
		void SwitchWeaponWielding() {
			ArmData@ src = null;
			ArmData@ dst = null;

			switch ( m_nHandsUsed ) {
			case LEFT_ARM:
			case BOTH_ARMS:
				@src = LeftArm;
				@dst = RightArm;
				break;
			case RIGHT_ARM:
				@src = RightArm;
				@dst = LeftArm;
				break;
			};

			if ( src.GetEquippedWeapon() == uint( -1 ) ) {
				// nothing in the source hand, deny
				return;
			}

			@LastUsedArm = @dst;

			const WeaponObject@ srcWeapon = @m_Inventory.GetSlot( src.GetEquippedWeapon() ).GetData();
			if ( srcWeapon.IsTwoHanded() && !srcWeapon.IsOneHanded() ) {
				// cannot change hands, no one-handing allowed
				return;
			}

			// check if the destination hand has something in it, if true, then swap
			if ( dst.GetEquippedWeapon() != uint( -1 ) ) {
				const uint tmp = dst.GetEquippedWeapon();

				dst.SetEquippedSlot( src.GetEquippedWeapon() );
				src.SetEquippedSlot( tmp );
			} else {
				// if we have nothing in the destination hand, then just clear the source hand
				const uint tmp = src.GetEquippedWeapon();

				src.SetEquippedSlot( uint( -1 ) );
				dst.SetEquippedSlot( tmp );
			}
		}
		void SwitchWeaponMode() {
			// weapon mode order (default)
			// blade -> blunt -> firearm

			WeaponSlot@ slot = null;

			switch ( m_nHandsUsed ) {
			case LEFT_ARM: {
				const uint nIndex = LeftArm.GetEquippedWeapon();
				if ( nIndex == uint( -1 ) ) {
					return;
				}
				@slot = m_Inventory.GetSlot( nIndex );
				break; }
			case RIGHT_ARM: {
				const uint nIndex = RightArm.GetEquippedWeapon();
				if ( nIndex == uint( -1 ) ) {
					return;
				}
				@slot = m_Inventory.GetSlot( nIndex );
				break; }
			case BOTH_ARMS:
				@slot = m_Inventory.GetEquippedWeapon();
				break;
			};

			const uint mode = slot.GetMode();
			const WeaponObject@ weapon = slot.GetData();
			
			slot.SetMode( InfoSystem::WeaponProperty::None ); // clear the modes
			
			// find the next most suitable mode
			const uint props = uint( weapon.GetWeaponInfo().weaponProps );
			const bool IsOneHanded = ( props & InfoSystem::WeaponProperty::IsOneHanded ) != 0;
			const bool isTwoHanded = ( props & InfoSystem::WeaponProperty::IsTwoHanded ) != 0;
			const uint hands = ~( InfoSystem::WeaponProperty::IsOneHanded | InfoSystem::WeaponProperty::IsTwoHanded );

			for ( uint i = 0; i < sgame_WeaponModeList.Count(); i++ ) {
				const uint current = sgame_WeaponModeList[i];
				if ( mode == current ) {
					// same mode, don't switch
					continue;
				}
				if ( ( props & ( current & hands ) ) != 0 ) {
					// apply some filters
					if ( ( current & InfoSystem::WeaponProperty::IsOneHanded ) != 0
						&& !IsOneHanded )
					{
						// one handing not possible, deny
						continue;
					}
					if ( ( current & InfoSystem::WeaponProperty::IsTwoHanded ) != 0
						&& !isTwoHanded )
					{
						// two handing not possible, deny
						continue;
					}
					
					// we've got a match!
					slot.SetMode( InfoSystem::WeaponProperty( current ) );
					slot.GetData().SetUseMode( current );
					break;
				}
			}
		}
		void SwitchUsedHand() {
			if ( TheNomad::Engine::IsKeyDown( TheNomad::Engine::KeyNum::V ) ) {
				m_nHandsUsed = BOTH_ARMS;
				@LastUsedArm = LeftArm;
				if ( LeftArm.GetEquippedWeapon() != uint( -1 ) ) {
					m_Inventory.EquipSlot( LeftArm.GetEquippedWeapon() );
				}
				return;
			}
			switch ( m_nHandsUsed ) {
			case LEFT_ARM:
				m_nHandsUsed = 1; // set to right
				@LastUsedArm = RightArm;
				if ( RightArm.GetEquippedWeapon() != uint( -1 ) ) {
					m_Inventory.EquipSlot( RightArm.GetEquippedWeapon() );
				}
				break;
			case RIGHT_ARM:
				m_nHandsUsed = 0; // set to left
				@LastUsedArm = LeftArm;
				if ( LeftArm.GetEquippedWeapon() != uint( -1 ) ) {
					m_Inventory.EquipSlot( LeftArm.GetEquippedWeapon() );
				}
				break;
			case BOTH_ARMS:
				m_nHandsUsed = 1;
				@LastUsedArm = RightArm;
				if ( RightArm.GetEquippedWeapon() != uint( -1 ) ) {
					m_Inventory.EquipSlot( RightArm.GetEquippedWeapon() );
				}
				break; // can't switch if we're using both hands for one weapon
			default:
				ConsoleWarning( "PlayrObject::SwitchUsedHand: invalid hand, setting to default of left.\n" );
				m_nHandsUsed = 0;
				break;
			};
		}

		InfoSystem::WeaponProperty GetCurrentWeaponMode() {
			switch ( m_nHandsUsed ) {
			case LEFT_ARM:
				if ( LeftArm.GetEquippedWeapon() == uint( -1 ) ) {
					return m_EmptyInfo.weaponProps;
				}
				return m_Inventory.GetSlot( LeftArm.GetEquippedWeapon() ).GetMode();
			case RIGHT_ARM:
			if ( RightArm.GetEquippedWeapon() == uint( -1 ) ) {
					return m_EmptyInfo.weaponProps;
				}
				return m_Inventory.GetSlot( RightArm.GetEquippedWeapon() ).GetMode();
			case BOTH_ARMS:
				return m_Inventory.GetEquippedWeapon().GetMode();
			default:
				break;
			};
			return InfoSystem::WeaponProperty::None;
		}

		void SetPlayerIndex( uint nIndex ) {
			m_nControllerIndex = nIndex;
		}
		uint GetPlayerIndex() const {
			return m_nControllerIndex;
		}

		const string& GetSkin() const {
			return m_Skin;
		}
		
		void DrawEmoteWheel() {
//			if ( !m_bEmoteWheelActive ) {
//				DebugPrint( "DrawEmoteWheel: called when inactive.\n" );
//				return;
//			}
			
		}

		void PassCheckpoint( EntityObject@ cp ) {
			MapCheckpoint@ data = cast<MapCheckpoint@>( cast<WallObject@>( cp ).GetData() );
			if ( data.m_bPassed ) {
				return;
			}

			EmitSound( TheNomad::Engine::SoundSystem::RegisterSfx( "event:/sfx/env/interaction/complete_checkpoint" ), 10.0f, 0xff );
			data.Activate( LevelManager.GetLevelTimer() );

			DebugPrint( "Setting checkpoint " + LevelManager.GetCheckpointIndex() + " to completed.\n" );
			
			TheNomad::Engine::CmdExecuteCommand( "sgame.save_game " + TheNomad::Engine::CvarVariableInteger( "sgame_SaveSlot" ) + "\n" );

			for ( uint i = 0; i < TheNomad::GameSystem::GameSystems.Count(); ++i ) {
				TheNomad::GameSystem::GameSystems[i].OnCheckpointPassed( i );
			}

			// done with the level?
			if ( data is MapCheckpoints[ MapCheckpoints.Count() - 1 ] ) {
				LevelManager.CalcLevelStats();
				GlobalState = GameState::LevelFinish;
				return;
			}
		}

		void EquipWeapon( WeaponObject@ weapon ) {
			EmitSound( ScreenData.m_EquipWeaponSfx, 10.0f, 0xff );
			m_Inventory.EquipWeapon( weapon );
		}

		void PickupItem( EntityObject@ item ) {
			ItemObject@ data = cast<ItemObject@>( item );
			data.SetOwner( this );

			switch ( item.GetType() ) {
			case TheNomad::GameSystem::EntityType::Item:
				break;
			case TheNomad::GameSystem::EntityType::Weapon:
				EquipWeapon( cast<WeaponObject@>( data ) );
				break;
			default:
				GameError( "PlayrObject::PickupItem: not an item" );
			};
		}

		uint& GetLegTicker() {
			return m_nLegTicker;
		}
		uint GetLegTicker() const {
			return m_nLegTicker;
		}

		bool IsSliding() const {
			return ( Flags & PF_SLIDING ) != 0;
		}
		void SetSliding( bool bSliding ) {
			if ( bSliding ) {
				Flags |= PF_SLIDING;
			} else {
				Flags &= ~PF_SLIDING;
			}
		}
		uint GetSlideTime() const {
			return SlideStartTime;
		}
		void ResetSlide() {
			SlideStartTime = ( TheNomad::GameSystem::GameTic + SLIDE_DURATION ) * TheNomad::GameSystem::DeltaTic;
		}
		
		bool IsCrouching() const {
			return ( Flags & PF_CROUCHING ) != 0;
		}
		void SetCrouching( bool bCrouching ) {
			if ( bCrouching ) {
				Flags |= PF_CROUCHING;
			} else {
				Flags &= ~PF_CROUCHING;
			}
		}

		bool IsDoubleJumping() const {
			return m_State.GetID() == StateNum::ST_PLAYR_DOUBLEJUMP;
		}

		bool IsDashing() const {
			return ( Flags & PF_DASHING ) != 0;
		}
		void SetDashing( bool bDashing ) {
			if ( bDashing ) {
				Flags |= PF_DASHING;
			} else {
				Flags &= ~PF_DASHING;
			}
		}
		uint GetDashTime() const {
			return DashStartTime;
		}
		void ResetDash() {
			DashStartTime = ( TheNomad::GameSystem::GameTic + DASH_DURATION ) * TheNomad::GameSystem::DeltaTic;
		}

		void SetUsingWeapon( bool bUseWeapon ) {
			Flags |= bUseWeapon ? PF_USING_WEAPON : 0;
		}
		void SetUsingWeaponAlt( bool bUseAltWeapon ) {
			Flags |= bUseAltWeapon ? PF_USING_WEAPON_ALT : 0;
		}
		void SetWeaponMode( InfoSystem::WeaponProperty nMode ) {
			m_Inventory.GetEquippedWeapon().SetMode( nMode );
		}
		void SetHandsUsed( int nHandsUsed ) {
			m_nHandsUsed = nHandsUsed;
		}
		void SetParryBoxWidth( float nParryBoxWidth ) {
			m_nParryBoxWidth = nParryBoxWidth;
		}
		void SetCurrentWeapon( int nCurrentWeapon ) {
			m_Inventory.EquipSlot( nCurrentWeapon );
		}

		bool UsingWeapon() const {
			return ( Flags & PF_USING_WEAPON ) != 0;
		}
		bool UsingWeaponAlt() const {
			return ( Flags & PF_USING_WEAPON_ALT ) != 0;
		}

		ArmData@ GetLeftArm() {
			return LeftArm;
		}
		ArmData@ GetRightArm() {
			return RightArm;
		}
		int GetHandsUsed() const {
			return m_nHandsUsed;
		}
		float GetParryBoxWidth() const {
			return m_nParryBoxWidth;
		}
		int GetCurrentWeaponIndex() const {
			return m_Inventory.GetSlotIndex();
		}

		void SetReflexMode( bool bReflex ) {
			m_nRawReflexStartTime = TheNomad::GameSystem::GameTic;
			if ( bReflex ) {
				Flags |= PF_REFLEX;
			} else {
				Flags &= ~PF_REFLEX;
			}
		}
		bool InReflex() const {
			return ( Flags & PF_REFLEX ) != 0;
		}

		SpriteSheet@ GetLeftArmSpriteSheet() {
			return LeftArm.GetSpriteSheet();
		}
		SpriteSheet@ GetRightArmSpriteSheet() {
			return RightArm.GetSpriteSheet();
		}
		const SpriteSheet@ GetLeftArmSpriteSheet() const {
			return LeftArm.GetSpriteSheet();
		}
		const SpriteSheet@ GetRightArmSpriteSheet() const {
			return RightArm.GetSpriteSheet();
		}

		void SetLegsFacing( int facing ) {
			m_LegsFacing = facing;
		}
		void SetLeftArmFacing( int facing ) {
			LeftArm.SetFacing( facing );
		}
		void SetRightArmFacing( int facing ) {
			RightArm.SetFacing( facing );
		}
		void SetArmsFacing( int facing ) {
			LeftArm.SetFacing( facing );
			RightArm.SetFacing( facing );
		}
		void SetArmsState( StateNum nState ) {
			LeftArm.SetState( nState );
			RightArm.SetState( nState );
		}
		void SetArmsState( EntityState@ state ) {
			LeftArm.SetState( state );
			RightArm.SetState( state );
		}
		int GetLegsFacing() const {
			return m_LegsFacing;
		}
		int GetLeftArmFacing() const {
			return LeftArm.GetFacing();
		}
		int GetRightArmFacing() const {
			return RightArm.GetFacing();
		}

		EntityState@ GetLegState() {
			return m_LegState;
		}
		EntityState@ GetLeftArmState() {
			return LeftArm.GetState();
		}
		const EntityState@ GetLegState() const {
			return m_LegState;
		}
		const EntityState@ GetLeftArmState() const {
			return LeftArm.GetState();
		}
		const EntityState@ GetRightArmState() const {
			return RightArm.GetState();
		}

		bool Load( const TheNomad::GameSystem::SaveSystem::LoadSection& in section ) {
			string skin;

			section.LoadString( "playerSkin", skin );
			TheNomad::Engine::CvarSet( "skin", skin );
			m_nHealth = section.LoadFloat( "health" );
			m_nRage = section.LoadFloat( "rage" );
			m_Link.m_Origin = section.LoadVec3( "origin" );

			m_Inventory.Load( section );
			m_nHandsUsed = section.LoadInt( "handsUsed" );

			return true;
		}
		void Save( const TheNomad::GameSystem::SaveSystem::SaveSection& in section ) const {
			uint index;
			
			section.SaveString( "playerSkin", TheNomad::Engine::CvarVariableString( "skin" ) );
			section.SaveFloat( "health", m_nHealth );
			section.SaveFloat( "rage", m_nRage );
			section.SaveVec3( "origin", m_Link.m_Origin );

			m_Inventory.Save( section );

			section.SaveInt( "handsUsed", m_nHandsUsed );
		}

		float GetArmAngle() const {
			return Pmove.m_nArmsAngle;
		}
		
		float GetHealthMult() const {
			return m_nHealMult;
		}
		float GetDamageMult() const {
			return m_nDamageMult;
		}
		float GetRage() const {
			return m_nRage;
		}

		void InvulOn() {
			Flags |= PF_INVUL;
		}
		void InvulOff() {
			Flags &= ~PF_INVUL;
		}
		
		void Damage( EntityObject@ attacker, float nAmount ) override {
			if ( m_bEmoting || ( Flags & PF_INVUL ) != 0 ) {
				return; // god has blessed thy soul...
			}

			m_HudData.ShowStatusBars();
			
			m_nHealth -= nAmount;
			m_nRage += nAmount * 0.01f;
			m_nDamageMult += nAmount;

			if ( m_nHealth <= 0.0f ) {
				if ( m_nFrameDamage > 0 ) {
					return; // as long as you're hitting SOMETHING, you cannot die
				}
				switch ( Util::PRandom() & 2 ) {
				case 0:
					EmitSound( ScreenData.m_DieSfx0, 10.0f, 0xff );
					break;
				case 1:
					EmitSound( ScreenData.m_DieSfx1, 10.0f, 0xff );
					break;
				case 2:
					EmitSound( ScreenData.m_DieSfx2, 10.0f, 0xff );
					break;
				};
				EntityManager.KillEntity( this, attacker );
				LevelManager.GetStats().numDeaths++;
				Flags = 0;
				
				Util::HapticRumble( m_nControllerIndex, 0.80f, 4000 );
			} else {
				switch ( Util::PRandom() & 2 ) {
				case 0:
					EmitSound( ScreenData.m_PainSfx0, 10.0f, 0xff );
					break;
				case 1:
					EmitSound( ScreenData.m_PainSfx1, 10.0f, 0xff );
					break;
				case 2:
					EmitSound( ScreenData.m_PainSfx2, 10.0f, 0xff );
					break;
				};
				
				Util::HapticRumble( m_nControllerIndex, 0.50f, 300 );

				GfxManager.AddBloodSplatter( m_Link.m_Origin, m_Facing );
			}
		}

		bool IsWeaponEquipped() const {
			const ArmData@ arm = null;
			switch ( m_nHandsUsed ) {
			case RIGHT_ARM:
				@arm = RightArm;
				break;
			case LEFT_ARM:
			case BOTH_ARMS:
				@arm = LeftArm;
				break;
			};
			return arm.GetEquippedWeapon() != uint( -1 ) ? true : false;
		}
		
		WeaponObject@ GetCurrentWeapon() {
			return m_Inventory.GetEquippedWeapon().GetData();
		}
		const WeaponObject@ GetCurrentWeapon() const {
			return m_Inventory.GetEquippedWeapon().GetData();
		}

		InventoryManager@ GetInventory() {
			return m_Inventory;
		}
		const InventoryManager@ GetInventory() const {
			return m_Inventory;
		}

		void SetLeftArmState( StateNum num ) {
			LeftArm.SetState( num );
		}
		void SetRightArmState( StateNum num ) {
			RightArm.SetState( num );
		}
		
		private float GetGfxDirection() const {
			if ( m_Facing == 1 ) {
				return -( m_Link.m_Bounds.m_nWidth / 2 );
			}
			return ( m_Link.m_Bounds.m_nWidth / 2 );
		}
		
		//
		// PlayrObject::CheckParry: called from DamageEntity mob v player
		//
		bool CheckParry( EntityObject@ ent ) {
			if ( ( Flags & PF_INVUL ) != 0 ) {
				return true;
			}
			if ( m_ParryBox.IntersectsBounds( ent.GetBounds() ) ) {
				if ( ent.IsProjectile() ) {
					// simply invert the direction and double the speed
					const vec3 v = ent.GetVelocity();
					ent.SetVelocity( vec3( v.x * 2, v.y * 2, v.z * 2 ) );
					ent.SetAngle( ent.GetAngle() * 2.0f / M_PI / 360.0f );
					ent.SetDirection( InverseDirs[ ent.GetDirection() ] );
				}
			}
			if ( ent.GetType() == TheNomad::GameSystem::EntityType::Mob && !ent.IsProjectile() ) {
				// just a normal counter
				MobObject@ mob = cast<MobObject@>( @ent );
				
				if ( !mob.CanParry() ) {
					// unblockable, deal damage
					return false;
				}
				else {
					// slap it back
					if ( mob.GetTicker() <= ( mob.GetTicker() / 4 ) ) {
						// counter parry, like in MGR, but more brutal, but for the future...
					}
					
					// TODO: make dead mobs with ANY velocity flying corpses
					EntityManager.DamageEntity( cast<EntityObject@>( @this ), ent );
					EmitSound( ScreenData.m_ParrySfx, 10.0f, 0xff );
				}
			}
			
			// add in the haptic
			Util::HapticRumble( m_nControllerIndex, 0.80f, 500 );
			
			// add the fireball
			GfxManager.AddExplosionGfx( vec3( m_Link.m_Origin.x + GetGfxDirection(), m_Link.m_Origin.y, m_Link.m_Origin.z ) );

			return true;
		}
		
		//
		// PlayrObject::ParryThink: calculates the bounds of the parry box
		//
		private void ParryThink() {
			m_ParryBox.m_nWidth = 2.5f + m_nParryBoxWidth;
			m_ParryBox.m_nHeight = 1.0f;
			m_ParryBox.MakeBounds( vec3( m_Link.m_Origin.x + ( m_Link.m_Bounds.m_nWidth / 2.0f ),
				m_Link.m_Origin.y, m_Link.m_Origin.z ) );

			if ( m_nParryBoxWidth >= 1.5f ) {
				return; // maximum
			}

			m_nParryBoxWidth += 0.5f;

			m_HudData.ShowStatusBars();
		}

		//
		// InitLoadout: initializes runtime player weapon data
		//
		private void InitLoadout() {
			m_EmptyInfo.name = "Fist (Empty)";
			m_EmptyInfo.type = ENTITYNUM_INVALID - 2;
			m_EmptyInfo.damage = 100.0f; // this is the god of war
			m_EmptyInfo.range = 1.5f;
			m_EmptyInfo.magSize = 0;
			m_EmptyInfo.weaponProps = InfoSystem::WeaponProperty( uint( InfoSystem::WeaponProperty::OneHandedBlunt ) );
			m_EmptyInfo.weaponType = InfoSystem::WeaponType::LeftArm;

			InfoSystem::InfoManager.GetWeaponTypes().Add( InfoSystem::EntityData( "weapon_fist", ENTITYNUM_INVALID - 2 ) );
			InfoSystem::InfoManager.GetWeaponInfos()[ "weapon_fist" ] = @m_EmptyInfo;
		}

		void Spawn( uint id, const vec3& in origin ) override {
			uvec2 torsoSheetSize, torsoSpriteSize;
			uvec2 armsSheetSize, armsSpriteSize;
			uvec2 legsSheetSize, legsSpriteSize;

			//
			// init all player data
			//
			m_Link.m_Origin = origin;
			m_Link.m_nEntityType = TheNomad::GameSystem::EntityType::Playr;
			m_Link.m_nEntityId = 0;
			m_nHealth = 100.0f;
			m_nRage = 0.0f;
			m_nHealMult = 10.0f;

			// give the player a little starting boost if they're playing on the easier modes
			if ( TheNomad::Engine::CvarVariableInteger( "sgame_Difficulty" ) == int( TheNomad::GameSystem::GameDifficulty::Easy ) ) {
				m_nRage = 50.0f;
			} else if ( TheNomad::Engine::CvarVariableInteger( "sgame_Difficulty" ) == int( TheNomad::GameSystem::GameDifficulty::Normal ) ) {
				m_nRage = 25.0f;
			}

			m_Direction = Util::Angle2Dir( m_PhysicsObject.GetAngle() );
			m_nHealMultDecay = LevelManager.GetDifficultyScale();

			InitLoadout();

			m_PhysicsObject.Init( cast<EntityObject>( @this ) );
			m_PhysicsObject.SetAngle( Util::Dir2Angle( TheNomad::GameSystem::DirType::East ) );
			m_PhysicsObject.SetWeight( TheNomad::Engine::CvarVariableFloat( "sgame_PlayerWeight" ) );

			// fetch the current skin's data
			m_Skin = TheNomad::Engine::CvarVariableString( "skin" );
			TheNomad::GameSystem::GetSkinData( m_Skin, m_SkinDescription, m_SkinDisplayName, torsoSheetSize, torsoSpriteSize,
				armsSheetSize, armsSpriteSize, legsSheetSize, legsSpriteSize );

			@m_SpriteSheet = TheNomad::Engine::ResourceCache.GetSpriteSheet( "skins/" +
				m_Skin, 1024, 1024, 32, 32 );
			if ( m_SpriteSheet is null ) {
				GameError( "PlayrObject::Spawn: failed to load torso sprite sheet" );
			}

			@m_State = StateManager.GetStateForNum( StateNum::ST_PLAYR_IDLE );
			if ( m_State is null ) {
				GameError( "PlayrObject::Spawn: failed to load idle torso state" );
			}
			m_Facing = FACING_RIGHT;

			LeftArm.Link( this, FACING_LEFT, armsSheetSize, armsSpriteSize );
			RightArm.Link( this, FACING_RIGHT, armsSheetSize, armsSpriteSize );

			@m_LegState = StateManager.GetStateForNum( StateNum::ST_PLAYR_LEGS_IDLE_GROUND );
			if ( m_LegState is null ) {
				GameError( "PlayrObject::Spawn: failed to load idle leg state" );
			}
			m_LegsFacing = FACING_RIGHT;

			m_nLegTicker = 0;
			m_Name = "player";

			@m_LegIdleState = StateManager.GetStateForNum( StateNum::ST_PLAYR_LEGS_IDLE_GROUND );
			@m_LegSlideState = StateManager.GetStateForNum( StateNum::ST_PLAYR_LEGS_SLIDE );
			@m_LegMoveState = StateManager.GetStateForNum( StateNum::ST_PLAYR_LEGS_MOVE_GROUND );
			@m_LegAirIdleState = StateManager.GetStateForNum( StateNum::ST_PLAYR_LEGS_IDLE_AIR );
			@m_LegAirMoveState = StateManager.GetStateForNum( StateNum::ST_PLAYR_LEGS_IDLE_GROUND );

			@m_IdleState = StateManager.GetStateForNum( StateNum::ST_PLAYR_IDLE );

			m_nHalfWidth = TheNomad::Engine::CvarVariableFloat( "sgame_PlayerWidth" ) * 0.5f;
			m_nHalfHeight = TheNomad::Engine::CvarVariableFloat( "sgame_PlayerHeight" ) * 0.5f;

			m_Link.m_nEntityType = TheNomad::GameSystem::EntityType::Playr;
		}
		void InitHUD() {
			m_HudData.Init( @this );
		}

		private void CheckStatus() {
			if ( m_nRage < 100.0f ) {
				if ( ( Flags & PF_USED_MANA ) == 0 ) {
//					m_nRage += 0.001f * TheNomad::GameSystem::DeltaTic;
					// this makes it WAY too easy
				}
				m_HudData.ShowRageBar();
			}
			if ( m_nFrameDamage > 0.0f ) {
				m_nRage += m_nFrameDamage * TheNomad::GameSystem::DeltaTic;
				m_nFrameDamage = 0.0f;
				Flags |= PF_USED_MANA;
				m_HudData.ShowRageBar();
			}
			if ( m_nHealth < 100.0f ) {
				m_nHealth += 0.075f * TheNomad::GameSystem::DeltaTic;
				m_nRage -= 0.5f * TheNomad::GameSystem::DeltaTic; // rage is essentially just converted mana
				// mana conversion ratio to health is extremely minimal

				Flags |= PF_USED_MANA;
				m_HudData.ShowHealthBar();
			}
			if ( m_nRage > 100.0f ) {
				m_nRage = 100.0f;
			} else if ( m_nRage < 0.0f ) {
				m_nRage = 0.0f;
			}
		}

		void Think() override {
			if ( ( uint( m_Flags ) & EntityFlags::Dead ) != 0 ) {
				if ( m_State.GetBaseNum() == StateNum::ST_PLAYR_DEAD_IDLE ) {
					@m_State = m_State.Run( m_nTicker );
				} else if ( m_State.GetBaseNum() == StateNum::ST_PLAYR_DEAD && m_State.Done( m_nTicker ) ) {
					@m_State = m_State.Run( m_nTicker );
				} else {
					m_State.GetAnimation().Run();
				}
				return;
			}
			TheNomad::GameSystem::IsRespawnActive = false;

			if ( m_State.GetID() == StateNum::ST_PLAYR_QUICKSHOT ) {
				m_QuickShot.Think();
			}

			WeaponObject@ current = GetCurrentWeapon();
			if ( current !is null ) {
				if ( ( Flags & PF_USING_WEAPON ) != 0 ) {
					m_nFrameDamage = current.Use( GetCurrentWeaponMode() );
				} else if ( ( Flags & PF_USING_WEAPON_ALT ) != 0 ) {
					m_nFrameDamage = current.UseAlt( GetCurrentWeaponMode() );
				}
				if ( current.GetFireMode() != InfoSystem::WeaponFireMode::Automatic ) {
					Flags &= ~( PF_USING_WEAPON | PF_USING_WEAPON_ALT );
				}
			}

			m_Link.m_Bounds.m_nWidth = sgame_PlayerWidth.GetFloat();
			m_Link.m_Bounds.m_nHeight = sgame_PlayerHeight.GetFloat();
			m_Link.m_Bounds.MakeBounds( m_Link.m_Origin );
			// update engine data
			m_Link.Update();
			
			//
			// check reflex mode
			//
			if ( ( Flags & PF_REFLEX ) != 0 ) {
				if ( ( TheNomad::GameSystem::GameTic - m_nRawReflexStartTime ) > RAW_REFLEX_TIME ) {
					// raw reflex time is done, start burning through mana
					m_nRawReflexStartTime = 0;
					Flags |= PF_USED_MANA;
					if ( m_nRage < 0.0f ) {
						Flags &= ~PF_REFLEX;
						EmitSound( ScreenData.m_SlowMoOff, 10.0f, 0xff );
						TheNomad::GameSystem::TIMESTEP = 1.0f / 60.0f;
					} else {
						m_nRage -= 8.0f * TheNomad::GameSystem::DeltaTic;
						m_HudData.ShowRageBar();
					}
				}
				
				/*
				if ( m_nRawReflexStartTime == 0 ) {
					if ( TheNomad::GameSystem::GameTic >= m_nReflexEndTime ) {
						m_nReflexEndTime = 0;
						m_nRawReflexStartTime = 0;
						m_HudData.SetReflexTime( 0.0f );
						m_HudData.SetReflexMode( false );
					}
					else {
						m_HudData.SetReflexTime( float( ( m_nReflexEndTime - TheNomad::GameSystem::GameTic ) ) * 0.01f );
					}
				}
				*/
			}

			// run a movement frame
			Pmove.RunTic();

			SetSoundPosition();

			if ( m_nHealth <= 15.0f ) {
				// if there's another haptic going on, we don't want to annihilate their hands
				Util::HapticRumble( m_nControllerIndex, 0.50f, 1000 );
			}

			CheckStatus();

			@m_LegState = m_LegState.Run( m_nLegTicker );
			LeftArm.Think();
			RightArm.Think();

			if ( m_nSoundLevel > 0.0f ) {
				m_nSoundLevel -= 1.0f * TheNomad::GameSystem::DeltaTic;
			}
			if ( m_nSoundLevel < 0.0f ) {
				m_nSoundLevel = 0.0f;
			}

			Flags &= ~PF_USED_MANA;
		}

		private void DrawLegs() {
			TheNomad::Engine::Renderer::RenderEntity refEntity;

			const vec3 velocity = m_PhysicsObject.GetVelocity();
			if ( m_Link.m_Origin.z > 0.0f ) {
				// airborne, special sprites
				if ( velocity.x == 0.0f && velocity.y == 0.0f ) {
					@m_LegState = m_LegAirIdleState;
				} else {
					@m_LegState = m_LegAirMoveState;
				}
			} else {
				if ( velocity.x == 0.0f && velocity.y == 0.0f ) {
					@m_LegState = m_LegIdleState;
				} else if ( IsSliding() ) {
					@m_LegState = m_LegSlideState;
				} else {
					@m_LegState = m_LegMoveState;
				}
			}

			refEntity.origin = m_Link.m_Origin;
			refEntity.sheetNum = m_SpriteSheet.GetShader();
			refEntity.spriteId = TheNomad::Engine::Renderer::GetSpriteId( m_SpriteSheet, m_LegState );
			refEntity.scale = TheNomad::Engine::Renderer::GetFacing( m_LegsFacing );
			refEntity.Draw();
		}
		
		// custom draw because of adaptive weapons and leg sprites
		void Draw() override {
		#if _NOMAD_DEBUG
			if ( sgame_DebugMode.GetBool() ) {
				TheNomad::Engine::ProfileBlock block( "PlayrObject::Draw" );
			}
		#endif

			TheNomad::Engine::Renderer::RenderEntity refEntity;

			if ( ( uint( m_Flags ) & EntityFlags::Dead ) != 0 ) {
				refEntity.origin = m_Link.m_Origin;
				refEntity.sheetNum = m_SpriteSheet.GetShader();
				refEntity.spriteId = TheNomad::Engine::Renderer::GetSpriteId( m_SpriteSheet, m_State );
				refEntity.scale = TheNomad::Engine::Renderer::GetFacing( m_Facing );
				refEntity.Draw();
				return;
			}

			@m_State = m_IdleState;

			ArmData@ back = null;
			ArmData@ front = null;

			switch ( m_Facing ) {
			case FACING_LEFT:
				@back = RightArm;
				@front = LeftArm;
				break;
			case FACING_RIGHT:
				@back = LeftArm;
				@front = RightArm;
				break;
			};
			if ( m_nHandsUsed == BOTH_ARMS ) {
				@front = LastUsedArm;
			} else {
				back.Draw();
			}
			refEntity.origin = m_Link.m_Origin;
			refEntity.sheetNum = m_SpriteSheet.GetShader();
			refEntity.spriteId = TheNomad::Engine::Renderer::GetSpriteId( m_SpriteSheet, m_State );
			refEntity.scale = TheNomad::Engine::Renderer::GetFacing( m_Facing );
			refEntity.Draw();

			if ( m_nHealth < 75.0f ) {
				refEntity.spriteId++;
			}
			if ( m_nHealth < 50.0f ) {
				refEntity.spriteId++;
			}
			if ( m_nHealth < 25.0f ) {
				refEntity.spriteId++;
			}
			refEntity.Draw();

			if ( ( Flags & PF_DASHING ) != 0 ) {
				vec3 origin = m_Link.m_Origin;

				switch ( m_LegsFacing ) {
				case FACING_LEFT:
					origin.x += 1.15f;
					break;
				case FACING_RIGHT:
					origin.x -= 1.15f;
					break;
				};
				origin.y += 0.5f;

				TheNomad::Engine::Renderer::AddDLightToScene( origin, 6.15f, 0.5f, 0.05f, 0.05f, 0.3f, vec3( 1.0f, 0.0f, 0.0f ) );
				GfxManager.SmokeCloud( m_Link.m_Origin );
			}

			DrawLegs();
			
			front.Draw();

			m_HudData.Draw();
//			m_StyleData.Draw();
		}

		PlayerDisplayUI@ GetUI() {
			return m_HudData;
		}

		KeyBind key_MoveNorth;
		KeyBind key_MoveSouth;
		KeyBind key_MoveEast;
		KeyBind key_MoveWest;
		KeyBind key_Jump;
		KeyBind key_Melee;

		private TheNomad::GameSystem::BBox m_ParryBox;
		private float m_nParryBoxWidth = 0.0f;

		InfoSystem::WeaponInfo m_EmptyInfo;

		// toggled with "sgame_SaveLastUsedWeaponModes"
		private InfoSystem::WeaponProperty[] m_WeaponModes( 9 );

		// used for various things but mostly just haptic feedback
		private uint m_nControllerIndex = 0;
		
		private LockShot m_QuickShot;

		// the amount of damage dealt in the frame
		private float m_nFrameDamage = 0.0f;
		
		// what does the player have in their hands?
		private int m_nHandsUsed = 0; // 0 for left, 1 for right, 2 if two-handed
		private uint m_nLeftArm = 0;
		private uint m_nRightArm = 0;

		// the lore goes: the harder and faster you hit The Nomad, the harder and faster he hits back
		private float m_nDamageMult = 0.0f;
		private float m_nRage = 0.0f;

		private float m_nHealMult = HEAL_MULT_BASE;
		private float m_nHealMultDecay = HEAL_MULT_BASE;

		ArmData@ LastUsedArm = @RightArm;
		
		ArmData LeftArm;
		ArmData RightArm;

		private EntityState@ m_IdleState = null;

		private EntityState@ m_LegSlideState = null;
		private EntityState@ m_LegIdleState = null;
		private EntityState@ m_LegMoveState = null;
		private EntityState@ m_LegAirIdleState = null;
		private EntityState@ m_LegAirMoveState = null;

		private EntityState@ m_LegState = null;
		private uint m_nLegTicker = 0;
		private uint m_LegsFacing = FACING_RIGHT;

		private AfterImage m_AfterImage;

		uint DashStartTime = 0;
		uint SlideStartTime = 0;

		private uint m_nRawReflexStartTime = 0;

		private bool m_bEmoting = false;
		
		private PlayerDisplayUI m_HudData;
		private StyleTracker m_StyleData;
		PMoveData Pmove( @this );

		uint Flags = 0;

		private InventoryManager m_Inventory( @this, @LeftArm, @RightArm );

		private int m_hListener;

		private string m_SkinDescription;
		private string m_SkinDisplayName;
		private string m_Skin = TheNomad::Engine::CvarVariableString( "skin" );
	};
};