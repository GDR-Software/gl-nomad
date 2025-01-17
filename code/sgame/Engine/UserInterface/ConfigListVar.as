#include "Engine/UserInterface/ConfigVar.as"

namespace TheNomad::Engine::UserInterface {
	class ConfigListVar : ConfigVar {
		ConfigListVar() {
		}
		ConfigListVar( const string& in name, const string& in id, const array<string>& in values, uint selected ) {
			m_nCurrent = selected;
			m_Values = values;
			
			super( name, id );
		}

		void Draw() {
			ImGui::TableNextColumn();

			ImGui::Text( m_Name );

			ImGui::SameLine();

			ImGui::TableNextColumn();

			if ( ImGui::BeginCombo( m_Name + "##" + m_Id, m_Values[ m_nCurrent ] ) ) {
				for ( uint i = 0; i < m_Values.Count(); i++ ) {
					if ( ImGui::Selectable( m_Values[ i ] + "##" + m_Id + i, ( m_nCurrent == i ) ) ) {
						m_nCurrent = i;
						m_bModified = true;
					}
				}
				ImGui::EndCombo();
			}

			ImGui::TableNextRow();
		}
		void Save() {
			TheNomad::Engine::CvarSet( m_Id, Convert().ToString( m_nCurrent ) );
		}

		private array<string> m_Values;
		private uint m_nCurrent = 0;
	};
};