// essentially assert() but in angelscript
void Assert( const bool condition, const string& in msg )
{
	if ( !condition ) {
		GameError( "Assertion failure: " + msg );
	}
}

namespace TheNomad::Util {
	void HapticRumble( uint nPlayerIndex, float nStrength, uint nTime ) {
		TheNomad::Engine::CmdExecuteCommand( "in_haptic_rumble " + nPlayerIndex + " " + nStrength + " " + nTime +  "\n" );
	}

	uint32 ColorAsUInt32( const vec4& in color ) {
		// convert color components to value between 0 and 255.
	    const uint32 r = 255 * uint32( color.r );
		const uint32 g = 255 * uint32( color.g );
	    const uint32 b = 255 * uint32( color.b );

    	// combine the color components in a single value of the form 0xAaBbGgRr
	    return 0xFF000000 | r | ( b << 16 ) | ( g << 8 );
	}

	array<string>@ ParseCSV( const string& in str ) {
	    array<string> values;
	    string data;

	    for ( uint i = 0; i < str.size(); i++ ) {
	        switch ( str[i] ) {
	        case ',':
	            values.Add( data );
	            data = "";
	            i++;
	            break;
	        case ' ':
	            break; // ignore it
	        default:
	            data += str[i];
	            break;
	        };
	    }
	
	    return @values;
	}

	const vec4& StringToColor( const string& in color ) {
		if ( StrICmp( color, "black" ) == 0 ) {
			return colorBlack;
		} else if ( StrICmp( color, "red" ) == 0 ) {
			return colorRed;
		} else if ( StrICmp( color, "green" ) == 0 ) {
			return colorGreen;
		} else if ( StrICmp( color, "yellow" ) == 0 ) {
			return colorYellow;
		} else if ( StrICmp( color, "blue" ) == 0 ) {
			return colorBlue;
		} else if ( StrICmp( color, "cyan" ) == 0 ) {
			return colorCyan;
		} else if ( StrICmp( color, "magenta" ) == 0 ) {
			return colorMagenta;
		} else if ( StrICmp( color, "gold" ) == 0 ) {
			return colorGold;
		} else if ( StrICmp( color, "white" ) == 0 ) {
			return colorWhite;
		} else {
			ConsoleWarning( "StringToColor: invalid color string \"" + color + "\"\n" );
		}
		return colorWhite;
	}

	float Swap( float a, float b ) {
		float tmp = a;
		a = b;
		b = tmp;
		return tmp;
	}
	int Swap( int a, int b ) {
		int tmp = 0;
		a = b;
		b = tmp;
		return tmp;
	}
	uint Swap( uint a, uint b ) {
		uint tmp = 0;
		a = b;
		b = tmp;
		return tmp;
	}

	//
	// M_Random
	// Returns a 0-255 number
	//

	const uint[] rndtable = {
		0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
	    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
	    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
	    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
	    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
	    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
	    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
	    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
	    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
	    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
	    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
	    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
	    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
	    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
	    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
	    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
	    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
	    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
	    120, 163, 236, 249
	};

	int	rndindex = 0;
	int	prndindex = 0;

	// Which one is deterministic?
	int PRandom() {
	    prndindex = ( prndindex + 1 ) & 0xff;
	    return rndtable[prndindex];
	}

	int MRandom() {
	    rndindex = ( rndindex + 1 ) & 0xff;
	    return rndtable[rndindex];
	}

	void MClearRandom() {
	    rndindex = prndindex = 0;
	}

	int Clamp( int value, int min, int max ) {
		if ( value > max ) {
			return max;
		} else if ( value < min ) {
			return min;
		}
		return value;
	}
	
	uint Clamp( uint value, uint min, uint max ) {
		if ( value > max ) {
			return max;
		} else if ( value < min ) {
			return min;
		}
		return value;
	}

	float Clamp( float value, float min, float max ) {
		if ( value > max ) {
			return max;
		} else if ( value < min ) {
			return min;
		}
		return value;
	}

	int Hex( char c ) {
		if ( c >= '0' && c <= '9' ) {
			return c - '0';
		} else if ( c >= 'A' && c <= 'F' ) {
			return 10 + c - 'A';
		} else if ( c >= 'a' && c <= 'f' ) {
			return 10 + c - 'a';
		}

		return -1;
	}

	int HexStringToInt( const string& in str ) {
		if ( str.size() < 1 ) {
			return -1;
		}

		// check for hex code
		if ( str[ 0 ] == '0' && str[ 1 ] == 'x' && str[ 2 ] != '\0' ) {
		    int i, digit, n = 0, len = str.size();

			for ( i = 2; i < len; i++ ) {
				n *= 16;

				digit = Hex( str[ i ] );

				if ( digit < 0 ) {
					return -1;
				}

				n += digit;
			}

			return n;
		}

		return -1;
	}

	bool GetHashColor( const string& in str, vec3& out color ) {
		uint i, len;
		int[] hex( 6 );

		color[0] = color[1] = color[2] = 0;

		if ( str[1] != '#' ) {
			return false;
		}

		len = str.size();
		if ( len <= 0 || len > 6 ) {
			return false;
		}

		for ( i = 1; i < len; i++ ) {
			hex[i] = Hex( str[i] );
			if ( hex[i] < 0 ) {
				return false;
			}
		}

		switch ( len ) {
		case 3: // #rgb
			color[0] = hex[0] << 4 | hex[0];
			color[1] = hex[1] << 4 | hex[1];
			color[2] = hex[2] << 4 | hex[2];
			break;
		case 6: // #rrggbb
			color[0] = hex[0] << 4 | hex[1];
			color[1] = hex[2] << 4 | hex[3];
			color[2] = hex[4] << 4 | hex[5];
			break;
		default: // unsupported format
			return false;
		};

		return true;
	}

	void CrossProduct( const vec3& in v1, const vec3& in v2, vec3& out cross ) {
		cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
		cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
		cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
	}

	float RadiusFromBounds( const TheNomad::GameSystem::BBox& in bounds ) {
		int		i;
		vec3	corner;

		// quiet down compiler
		float	a = 0.0f;
		float	b = 0.0f;

		for ( i = 0; i < 3; i++ ) {
			a = abs( bounds.m_Mins[i] );
			b = abs( bounds.m_Maxs[i] );
			corner[i] = a > b ? a : b;
		}

		return VectorLength( corner );
	}

	void VectorScale( const vec3& in src, float scale, vec3& out dst ) {
		dst[0] = src[0]*scale;
		dst[1] = src[1]*scale;
		dst[2] = src[2]*scale;
	}

	float DotProduct( const vec3& in a, const vec3& in b ) {
		return ( a[0]*b[0] + a[1]*b[1] + a[2]*b[2] );
	}

	float VectorLength( const vec3& in v ) {
		return sqrt( DotProduct( v, v ) );
	}

	float VectorNormalize( vec3& out v ) {
		float length, ilength;

		length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

		if ( length > 0.0f ) {
			ilength = 1.0f / float( sqrt( length ) );
			/* sqrt(length) = length * (1 / sqrt(length)) */
			length *= ilength;
			v[0] *= ilength;
			v[1] *= ilength;
			v[2] *= ilength;
		}
		
		return length;
	}


	int8 ClampChar( int8 i ) {
		if ( i < -128 ) {
			return -128;
		}
		if ( i > 127 ) {
			return 127;
		}
		return i;
	}

	int8 ClampCharMove( int8 i ) {
		if ( i < -127 ) {
			return -127;
		}
		if ( i > 127 ) {
			return 127;
		}
		return i;
	}

	int16 ClampShort( int16 i ) {
		if ( i < -32768 ) {
			return -32768;
		}
		if ( i > 0x7fff ) {
			return 0x7fff;
		}
		return i;
	}

	bool IsPrint( int c ) {
		if ( c >= 0x20 && c <= 0x7E ) {
			return true;
		}
		return false;
	}

	bool IsLower( int c ) {
		if ( c >= 'a' && c <= 'z' ) {
			return true;
		}
		return false;
	}

	bool IsUpper( int c ) {
		if ( c >= 'A' && c <= 'Z' ) {
			return true;
		}
		return false;
	}

	bool IsAlpha( int c ) {
		if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) {
			return true;
		}
		return false;
	}

	bool IsIntegral( float f ) {
		return ( int( f ) == f );
	}

	float Lerp( float a, float b, float f ) {
		return ( a * ( 1.0f - f ) ) + ( b * f );
	}

	float DEG2RAD( float x ) {
		return ( ( x * M_PI ) / 180.0F );
	}

	float RAD2DEG( float x ) {
		return ( ( x * 180.0f ) / M_PI );
	}

	//
	// Dir2Angle: returns absolute degrees
	//
	float Dir2Angle( TheNomad::GameSystem::DirType dir ) {
		switch ( dir ) {
		case TheNomad::GameSystem::DirType::North:
			return 0.0f;
		case TheNomad::GameSystem::DirType::NorthEast:
			return 45.0f;
		case TheNomad::GameSystem::DirType::East:
			return 90.0f;
		case TheNomad::GameSystem::DirType::SouthEast:
			return 135.0f;
		case TheNomad::GameSystem::DirType::South:
			return 180.0f;
		case TheNomad::GameSystem::DirType::SouthWest:
			return 225.0f;
		case TheNomad::GameSystem::DirType::West:
			return 270.0f;
		case TheNomad::GameSystem::DirType::NorthWest:
			return 315.0f;
		default:
			GameError( "Dir2Angle: invalid dir " + formatUInt( dir ) );
		};
		return -1.0f;
	}

	//
	// Angle2Dir:
	//
	TheNomad::GameSystem::DirType Angle2Dir( float angle ) {
		if ( ( angle >= 337.5f && angle <= 360.0f ) || ( angle >= 0.0f && angle <= 22.5f ) ) {
			return TheNomad::GameSystem::DirType::North;
		} else if ( angle >= 22.5f && angle <= 67.5f ) {
			return TheNomad::GameSystem::DirType::NorthEast;
		} else if ( angle >= 67.5f && angle <= 112.5f ) {
			return TheNomad::GameSystem::DirType::East;
		} else if ( angle >= 112.5f && angle <= 157.5f ) {
			return TheNomad::GameSystem::DirType::SouthEast;
		} else if ( angle >= 157.5f && angle <= 202.5f ) {
			return TheNomad::GameSystem::DirType::South;
		} else if ( angle >= 202.5f && angle <= 247.5f ) {
			return TheNomad::GameSystem::DirType::SouthWest;
		} else if ( angle >= 247.5f && angle <= 292.5f ) {
			return TheNomad::GameSystem::DirType::West;
		} else if ( angle >= 292.5f && angle <= 337.5f ) {
			return TheNomad::GameSystem::DirType::NorthWest;
		} else {
			DebugPrint( "Angle2Dir: funny angle " + formatFloat( angle ) + "\n" );
		}
		return TheNomad::GameSystem::DirType::North;
	}
};