namespace TheNomad::SGame::InfoSystem {
	enum ItemType {
		Powerup,
		Consumable,

		NumItemTypes
	};

    enum AttackMethod {
		Hitscan,
		RayCast, // for melee without the AOE
		Projectile,
		AreaOfEffect,

		NumAttackMethods
	};

	enum ArmorType {
		None,
		Light,
		Standard,
		Heavy,
		Invul,

		NumArmorTypes
	};

	enum AmmoType {
		Bullet,
		Shell,
		Rocket,
		Grenade,
		Invalid,

		NumAmmoTypes,
	};

	enum AttackType {
		Melee,
		Missile,

		NumAttackTypes
	};
	
	enum WeaponType {
		Sidearm,
		HeavySidearm,
		Primary,
		HeavyPrimary,
		Grenadier,
		Melee,
		LeftArm,
		RightArm,
		
		NumWeaponTypes
	};

	enum MobFlags {
		Deaf		= 0x0001,
		Blind		= 0x0002,
		Terrified	= 0x0004,
		Boss		= 0x0008,
		Sentry		= 0x0010,
		PermaDead	= 0x0020,
		Leader		= 0x004,

		None      = 0x0000
	};

	enum AmmoProperty {
		Heavy				= 0x0001, // 5.56/7.62/.223 REM
		Light				= 0x0002, // bullet/9mm/45 ACP
		Pellets				= 0x0003, // 12 gauge/20 gauge/8 gauge
		TypeBits			= 0x000f,

		NoPenetration		= 0x0010, // very little penetration, mostly for pistol ammunition
		ArmorPiercing		= 0x0020, // armor piercing specialized ammo, can bypass light to medium armor
		HollowPoint			= 0x0030, // does extra damage against light armor but less damage against heavy armor
		PenetrationBits		= 0x00f0,

		Flechette			= 0x0100, // warcrime, applies bleed status effect
		Buckshot			= 0x0200, // effective against heavy armor enemies or densly packed enemies, but weak at croud-control
		Birdshot			= 0x0300, // extremely effective at light to medium armor croud-control, almost useless against heavy armor
		Shrapnel			= 0x0400, // warcrime, effective against light armor enemies but weak against anything heavier, cheap however
		Slug				= 0x0500, // extremely effective on medium armor enemies, but not good as a croud-control or spray-and-pray
		ShotgunBullshitBits	= 0x0f00,

		Explosive			= 0x1000, // EXPLOOOOOOSION!
		Incendiary			= 0x2000, // dragon's breath, warcrime
		Tracer				= 0x3000, // tracers
		SubSonic			= 0x4000, // extra stealth modifier, but less damage
		ExtBulletBits		= 0xf000,

		None				= 0x0000 // invalid ammo
	};

	enum WeaponProperty {
		OneHandedBlade       = 0x00000101,
		OneHandedBlunt       = 0x00001002,
		OneHandedPolearm     = 0x00010003,
		OneHandedSideFirearm = 0x00100004,
		OneHandedPrimFirearm = 0x00100005,

		TwoHandedBlade       = 0x00000110,
		TwoHandedBlunt       = 0x00001020,
		TwoHandedPolearm     = 0x00010030,
		TwoHandedSideFirearm = 0x00100040,
		TwoHandedPrimFirearm = 0x00100050,

		IsOneHanded          = 0x0000000f,
		IsTwoHanded          = 0x000000f0,
		IsBladed             = 0x00000f00,
		IsBlunt              = 0x0000f000,
		IsPolearm            = 0x000f0000,
		IsFirearm            = 0x00f00000,

		SpawnsObject         = 0x10000000,

		None                 = 0x00000000
	};

	const string[] ItemTypeStrings = {
		"Powerup",
		"Consumable"
	};

	const uint[] WeaponPropertyBits = {
		uint( WeaponProperty::TwoHandedBlade ),
		uint( WeaponProperty::OneHandedBlade ),
		uint( WeaponProperty::TwoHandedBlunt ),
		uint( WeaponProperty::OneHandedBlunt ),

		uint( WeaponProperty::TwoHandedPolearm ),
		uint( WeaponProperty::OneHandedPolearm ),

		uint( WeaponProperty::OneHandedSideFirearm ),
		uint( WeaponProperty::TwoHandedSideFirearm ),
		uint( WeaponProperty::OneHandedPrimFirearm ),
		uint( WeaponProperty::TwoHandedPrimFirearm ),

		uint( WeaponProperty::SpawnsObject )
	};

	const string[] WeaponPropertyStrings = {
		"TwoHandedBlade",
		"OneHandedBlade",
		"TwoHandedBlunt",
		"OneHandedBlunt",

		"TwoHandedPolearm",
		"OneHandedPolearm",

		"OneHandedSideFirearm",
		"TwoHandedSideFirearm",
		"OneHandedPrimFirearm",
		"TwoHandedPrimFirearm",

		"SpawnsObject"
	};

	const string[] AmmoPropertyStrings = {
		"Heavy",
		"Light",
		"Pellets",

		"NoPenetration",
		"ArmorPiercing",
		"HollowPoint",

		"Flechette",
		"Buckshot",
		"Birdshot",
		"Shrapnel",
		"Slug",

		"Explosive",
		"Incendiary",
		"Tracer",
		"SubSonic"
	};

	const string[] AmmoTypeStrings = {
		"Bullet",
		"Shell",
		"Grenade",
		"Rocket"
	};
	const string[] ArmorTypeStrings = {
		"None",
		"Light",
		"Standard",
		"Heavy",
		"Invul"
	};
	const string[] WeaponTypeStrings = {
		"Sidearm",
		"HeavySidearm",
		"Primary",
		"HeavyPrimary",
		"Grenadier",
		"Melee",
		"LeftArm",
		"RightArm"
	};
};