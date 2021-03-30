#pragma once


#include <ShlObj_core.h>
#include "../utils.h"

#include "json/json.hpp"
#include "../../singleton.h"

using json = nlohmann::json;

class manager : public c_singleton <manager>
{
	class item {
	public:
		std::string name;
		void* pointer;
		std::string type;

		item( std::string name, void* pointer, std::string type )
		{
			this->name = name;
			this->pointer = pointer;
			this->type = type;
		}
	};
public:
protected:

	std::vector<item*> items;

public:

	void setup_config( );

	void add_item( void* pointer, const char* name, std::string type )
	{
		items.push_back( new item( std::string( name ), pointer, type ) );
	}

	void setup_item( int* pointer, int value, std::string name )
	{
		add_item( pointer, name.c_str( ), "int" );
		*pointer = value;
	}

	void setup_item( bool* pointer, bool value, std::string name )
	{
		add_item( pointer, name.c_str( ), "bool" );
		*pointer = value;
	}

	void setup_item( float* pointer, float value, std::string name )
	{
		add_item( pointer, name.c_str( ), "float" );
		*pointer = value;
	}

	void setup_item( DWORD* pointer, DWORD value, std::string name )
	{
		add_item( pointer, name.c_str( ), "dword" );
		*pointer = value;
	}

	void save( std::string config )
	{
		std::string folder, file;

		auto get_dir = [ &folder, &file, &config ]( ) ->void {
			static TCHAR path[ MAX_PATH ];
			if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, path ) ) ) {
				folder = std::string( path ) + "\\xyu.cc\\";
				file = std::string( path ) + "\\xyu.cc\\" + config;
			}

			CreateDirectory( folder.c_str( ), NULL );
		};
		get_dir( );

		std::ofstream ofs;

		ofs.open( file + "", std::ios::out | std::ios::trunc );

		json allJson;

		for ( auto it : items ) {
			json j;

			j[ "name" ] = it->name;
			j[ "type" ] = it->type;

			if ( !it->type.compare( "int" ) ) {
				j[ "value" ] = ( int )*( int* )it->pointer;
			}
			else if ( !it->type.compare( "float" ) ) {
				j[ "value" ] = ( float )*( float* )it->pointer;
			}
			else if ( !it->type.compare( "bool" ) ) {
				j[ "value" ] = ( bool )*( bool* )it->pointer;
			}
			else if ( !it->type.compare( "dword" ) ) {
				j[ "value" ] = ( DWORD )*( DWORD* )it->pointer;
			}

			allJson.push_back( j );
		}

		std::string data = allJson.dump( );

		ofs << std::setw( 4 ) << data << std::endl;

		ofs.close( );
	}

	void load( std::string config )
	{
		static auto find_item = [ ]( std::vector< item* > items, std::string name ) -> item* {
			for ( int i = 0; i < ( int )items.size( ); i++ ) {
				if ( items[ i ]->name.compare( name ) == 0 )
					return ( items[ i ] );
			}

			return nullptr;
		};

		static auto right_of_delim = [ ]( std::string const& str, std::string const& delim ) -> std::string {
			return str.substr( str.find( delim ) + delim.size( ) );
		};

		std::string folder, file;

		auto get_dir = [ &folder, &file, &config ]( ) ->void {
			static TCHAR path[ MAX_PATH ];
			if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, path ) ) ) {
				folder = std::string( path ) + "\\xyu.cc\\";
				file = std::string( path ) + "\\xyu.cc\\" + config;
			}

			CreateDirectory( folder.c_str( ), NULL );
		};

		get_dir( );

		std::ifstream ifs;
		std::string data;

		std::string path = file + "";

		ifs.open( path );

		json allJson;

		ifs >> allJson;

		for ( json::iterator it = allJson.begin( ); it != allJson.end( ); ++it ) {
			json j = *it;

			std::string name = j[ "name" ];
			std::string type = j[ "type" ];

			item* m_item = find_item( items, name );

			if ( m_item ) {
				if ( !type.compare( "int" ) ) {
					*( int* )m_item->pointer = j[ "value" ].get<int>( );
				}
				else if ( !type.compare( "float" ) ) {
					*( float* )m_item->pointer = j[ "value" ].get<float>( );
				}
				else if ( !type.compare( "bool" ) ) {
					*( bool* )m_item->pointer = j[ "value" ].get<bool>( );
				}
				else if ( !type.compare( "dword" ) ) {
					*( DWORD* )m_item->pointer = j[ "value" ].get<DWORD>( );
				}
			}
		}

		ifs.close( );
	}

	std::vector<std::string> files;

	void config_files( )
	{
		std::string folder;

		auto get_dir = [ &folder ]( ) -> void {
			static TCHAR path[ MAX_PATH ];
			if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, path ) ) ) {
				folder = std::string( path ) + "\\xyu.cc\\";
			}

			CreateDirectory( folder.c_str( ), NULL );
		};

		get_dir( );

		files.clear( );

		std::string path = folder + "/*.json";// "/*.*";

		WIN32_FIND_DATA fd;

		HANDLE hFind = ::FindFirstFile( path.c_str( ), &fd );

		if ( hFind != INVALID_HANDLE_VALUE ) {
			do {
				if ( !( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
					files.push_back( fd.cFileName );
				}
			} while ( ::FindNextFile( hFind, &fd ) );

			::FindClose( hFind );
		}
	}
};

#define master manager::instance( )

enum m_cfg_weapon
{
	m_awp,
	m_scout,
	m_heavy,
	m_auto,
	m_rifle,
	m_smg,
	m_shotgun,
	m_pistols
};

class config
{
public:
	struct
	{
		bool enabled;

		int target_selection;

		int auto_scope;
		bool autofire;
		bool norecoil;

		bool hitchance;
		float hitchance_amount;

		int visible_minimal_damage;
		bool visible_minimal_damage_hp;
		int minimal_damage_override;
		bool minimal_damage_override_pen;
		bool penetration;
		int penetration_minimal_damage;
		bool penetration_minimal_damage_hp;

		int prefered_hitbox;
		bool prefer_safepoint;

		bool hitbox_head;
		bool hitbox_neck;
		bool hitbox_chest;
		bool hitbox_stomach;
		bool hitbox_arms;
		bool hitbox_legs;
		bool hitbox_feet;

		int multipoint_head_scale;
		int multipoint_stomach_scale;
		bool multipoint_head;
		bool multipoint_feet;
		bool multipoint_stomach;
		bool multipoint_chest;
		bool multipoint_legs;

		bool baim_prefer_always;
		bool baim_prefer_lethal;
		bool baim_prefer_air;

		bool baim_always;
		bool baim_always_health; int baim_always_health_amount;
		bool baim_always_air;

		struct
		{
			bool enabled;
			int exploit_type;
			bool wait_charge;
		} main;

	} ragebot;

	struct
	{
		int inver_key = -1;
		int fakeduck_key = -1;
		int slowwalk_key = -1;

		bool enabled;
		int pitch;
		int yaw;
		int base_angle;
		int jitter_type;
		int jitter_range;
		bool fake;
		int max_fake_delta;

		bool enabled_around_fake_jitter;
		bool real_around_fake_standing;
		bool real_around_fake_moving;
		bool real_around_fake_air;
		bool real_around_fake_slow_motion;

		bool fake_lag_enabled;

		int fake_lag_mode;
		int fake_lag_limit = 2;

		bool fake_lag_on_stand;
		bool fake_lag_on_moving;
		bool fake_lag_on_air;

		bool fake_lag_on_peek;
	} hvh;

	struct
	{
		bool unlock_inventory;
	} misc;
};

extern config m_cfg;