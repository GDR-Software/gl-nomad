#include "SGame/EntityState.as"
#include "SGame/EntityStateSystem.as"
#include "SGame/PlayrObject.as"
#include "Engine/Physics/PhysicsObject.as"
#include "Engine/SoundSystem/SoundEmitter.as"

namespace TheNomad::SGame {
	const int FACING_RIGHT = 0;
	const int FACING_LEFT = 1;
	const int FACING_UP = 2;
	const int FACING_DOWN = 3;
	const int NUMFACING = 2;

	const uint WALL_CHECKPOINT_ID = 0;

    class EntityObject {
		EntityObject( TheNomad::GameSystem::EntityType type, uint id, const vec3& in origin ) {
			Init( type, id, origin, 0 );
		}
		EntityObject() {
		}
		
		void Init( TheNomad::GameSystem::EntityType type, uint id, const vec3& in origin, uint nEntityNumber ) {
			// just create a temporary bbox to link it in, we'll rebuild every frame anyway
			TheNomad::GameSystem::BBox bounds( 1.0f, 1.0f, origin );
			m_Link.Create( origin, bounds, id, uint( type ), nEntityNumber );
			m_PhysicsObject.Init( @this );

			m_Link.m_nEntityId = id;
			m_Link.m_nEntityType = type;

			// create emitter
			m_hEmitter = TheNomad::Engine::SoundSystem::RegisterEmitter( m_Link.m_nEntityNumber );
		}
		void EmitSound( const TheNomad::Engine::SoundSystem::SoundEffect hSfx, float nVolume, uint nListenerMask ) {
			TheNomad::Engine::SoundSystem::PlayEmitterSound( m_hEmitter, nVolume, nListenerMask, hSfx );
		}
		void SetSoundPosition() {
			TheNomad::Engine::SoundSystem::SetEmitterPosition( m_hEmitter, m_Link.m_Origin, 0.5f, 0.0f,
				m_PhysicsObject.GetAcceleration().x + m_PhysicsObject.GetAcceleration().y );
		}
		void SetVolume( float nVolume ) {
			TheNomad::Engine::SoundSystem::SetEmitterVolume( m_hEmitter, nVolume );
		}

		void RunState() {
			@m_State = m_State.Run( m_nTicker );
		}
		
		//
		// getters
		//
		bool StateDone() const {
			return m_State.Done( m_nTicker );
		}
		uint& GetTicker() {
			return m_nTicker;
		}
		uint GetTicker() const {
			return m_nTicker;
		}
		float GetAngle() const {
			return m_PhysicsObject.GetAngle();
		}
		float GetHealth() const {
			return m_nHealth;
		}
		const string& GetName() const {
			return m_Name;
		}
		const EntityState@ GetState() const {
			return @m_State;
		}
		EntityState@ GetState() {
			return @m_State;
		}
		int GetShader() const {
			return m_hShader;
		}
		uint GetFlags() const {
			return m_Flags;
		}
		int GetFacing() const {
			return m_Facing;
		}
		uint GetId() const {
			return m_Link.m_nEntityId;
		}
		uint GetEntityNum() const {
			return m_Link.m_nEntityNumber;
		}
		TheNomad::GameSystem::BBox& GetBounds() {
			return m_Link.m_Bounds;
		}
		const TheNomad::GameSystem::LinkEntity& GetLink() const {
			return m_Link;
		}
		TheNomad::GameSystem::LinkEntity& GetLink() {
			return m_Link;
		}
		TheNomad::GameSystem::DirType GetDirection() const {
			return m_Direction;
		}
		TheNomad::Engine::Physics::PhysicsObject& GetPhysicsObject() {
			return m_PhysicsObject;
		}
		InfoSystem::InfoLoader@ GetInfo() {
			return m_InfoData;
		}
		vec3& GetOrigin() {
			return m_Link.m_Origin;
		}
		const vec3& GetOrigin() const {
			return m_Link.m_Origin;
		}
		const vec3& GetVelocity() const {
			return m_PhysicsObject.GetVelocity();
		}
		vec3& GetVelocity() {
			return m_PhysicsObject.GetVelocity();
		}
		const vec3& GetAcceleration() const {
			return m_PhysicsObject.GetAcceleration();
		}
		void SetAcceleration( const vec3& in accel ) {
			m_PhysicsObject.SetAcceleration( accel );
		}
		int GetWaterLevel() const {
			return m_PhysicsObject.GetWaterLevel();
		}
		void SetWaterLevel( int level ) {
			m_PhysicsObject.SetWaterLevel( level );
		}
		float GetSoundLevel() const {
			return m_nSoundLevel;
		}
		TheNomad::GameSystem::EntityType GetType() const {
			return TheNomad::GameSystem::EntityType( m_Link.m_nEntityType );
		}
		bool IsProjectile() const {
			return ( uint( m_Flags ) & EntityFlags::Projectile ) != 0;
		}
		SpriteSheet@ GetSpriteSheet() {
			return m_SpriteSheet;
		}
		const SpriteSheet@ GetSpriteSheet() const {
			return m_SpriteSheet;
		}
		float GetHalfWidth() const {
			return m_nHalfWidth;
		}
		float GetHalfHeight() const {
			return m_nHalfHeight;
		}

		//
		// setters
		//
		void SetHealth( float nHealth ) {
			m_nHealth = nHealth;
		}
		void SetState( StateNum stateNum ) {
			@m_State = @StateManager.GetStateForNum( stateNum );
			m_State.Reset( m_nTicker );
		}
		void SetState( EntityState@ state ) {
			@m_State = @state;
			if ( @m_State is null ) {
				GameError( "null\n" );
				return;
			}
			m_State.Reset( m_nTicker );
		}
		void SetFlags( uint flags ) {
			m_Flags = EntityFlags( flags );
		}
		void SetProjectile( bool bProjectile ) {
			if ( bProjectile ) {
				m_Flags = EntityFlags( uint( m_Flags ) | EntityFlags::Projectile );
			} else {
				m_Flags = EntityFlags( uint( m_Flags ) & ~EntityFlags::Projectile );
			}
		}
		void SetVelocity( const vec3& in vel ) {
			m_PhysicsObject.SetVelocity( vel );
		}
		void SetOrigin( const vec3& in origin ) {
			m_Link.m_Origin = origin;
		}
		void SetDirection( TheNomad::GameSystem::DirType dir ) {
			m_Direction = dir;
		}
		void SetFacing( int facing ) {
			m_Facing = facing;
		}
		void SetAngle( float nAngle ) {
			m_PhysicsObject.SetAngle( nAngle );
		}
		void SetSoundLevel( float nSoundLevel ) {
			m_nSoundLevel = nSoundLevel;
		}

		//
		// functions that should be implemented by the derived class
		//

		void PickupItem( EntityObject@ item ) {
		}
		
		//
		// EntityObject::Load: should only return false if we're missing something
		// in the save data
		//
		bool Load( const TheNomad::GameSystem::SaveSystem::LoadSection& in section ) {
			ConsoleWarning( "EntityObject::Load: called\n" );
			return true;
		}
		void Save( const TheNomad::GameSystem::SaveSystem::SaveSection& in section ) const {
			ConsoleWarning( "EntityObject::Save: callled\n" );
		}
		void Damage( EntityObject@ attacker, float nAmount ) {
			ConsoleWarning( "EntityObject::Damage: called\n" );
		}
		void Think() {
			ConsoleWarning( "EntityObject::Think: called\n" );
		}
		void Draw() {
		}
		void Spawn( uint, const vec3& in ) {
			ConsoleWarning( "EntityObject::Spawn: called\n" );
		}

		//
		// utilities
		//
		bool CheckFlags( uint flags ) const {
			return ( m_Flags & flags ) != 0;
		}

		// mostly meant for debugging
		protected string m_Name;

		// used for physics in the modules
		protected TheNomad::Engine::Physics::PhysicsObject m_PhysicsObject;

		// engine data, for physics
		protected TheNomad::GameSystem::LinkEntity m_Link;

		// cached info
		protected InfoSystem::InfoLoader@ m_InfoData = null;

		// the entity's current state
		protected EntityState@ m_State = null;
		
		// DUH.
		protected float m_nHealth = 0.0f;
		
		// flags, some are specific
		protected EntityFlags m_Flags = EntityFlags::None;
		
		// sound emitter
		protected int m_hEmitter = FS_INVALID_HANDLE;

		protected float m_nHalfWidth = 1.0f;
		protected float m_nHalfHeight = 1.0f;
		
		protected TheNomad::GameSystem::DirType m_Direction = TheNomad::GameSystem::DirType::North;

		protected float m_nSoundLevel = 0.0f;
		
		// for direction based sprite drawing
		protected int m_Facing = 0;
		
		//
		// renderer data
		//
		protected int m_hShader = FS_INVALID_HANDLE;
		protected SpriteSheet@ m_SpriteSheet = null;
		
		// linked list stuff
		EntityObject@ m_Next = null;
		EntityObject@ m_Prev = null;

		protected uint m_nTicker = 0;
	};
};