namespace moblib::System {
	enum SensorType {
		Sensor_Sight,
		Sensor_Sound
	};
	
	class AISensor {
		AISensor() {
		}

		void Init( TheNomad::SGame::InfoSystem::MobInfo@ info, TheNomad::SGame::MobObject@ mob ) {
			@m_EntityData = @mob;	
		}
		
		bool CheckSensor() {
			if ( m_Eyes.DoCheck( @m_EntityData ) ) {
				
			}
			if ( m_Ears.DoCheck( @m_EntityData ) ) {
				
			}
			return false;
		}
		void Stimulate( SensorType nType ) {
		}
		
		private float m_nSuspicion = 0.0f;
		private AISensorSight m_Eyes;
		private AISensorSound m_Ears;
		private TheNomad::SGame::AfterImage m_TargetAfterImage;
		private TheNomad::SGame::MobObject@ m_EntityData = null;
	};
};