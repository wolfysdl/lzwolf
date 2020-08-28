/*
** am_map.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "wl_def.h"
#include "am_map.h"
#include "colormatcher.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "r_data/colormaps.h"
#include "r_sprites.h"
#include "v_font.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"
#include "mapedit.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "v_text.h"

#include <cstddef>
#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>       
#include <fcntl.h>          
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <deque>
#include <boost/optional.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h> 
#include <sys/shm.h> 

static constexpr auto shared_segment_size = 0x6400;
static constexpr auto SemaphoreName = "mysemaphore";
static constexpr auto AccessPerms = 0644;

class CSharedMemReader
{
public:
    void report_and_exit( const char* msg )
    {
        perror( msg );
        exit( -1 );
    }

    explicit CSharedMemReader( key_t shmkey )
    {
        /* Allocate a shared memory segment.  */
        segment_id = shmget( shmkey, shared_segment_size,
                             IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR );
        if( segment_id == -1 )
        {
            report_and_exit( "shmget" );
        }
        /* Attach the shared memory segment.  */
        shared_memory = ( char* ) shmat( segment_id, 0, 0 );
        if( shared_memory == ( char* ) -1 )
        {
            report_and_exit( "shmat" );
        }
        printf( "shared memory attached at address %p\n", shared_memory );
    }

    ~CSharedMemReader()
    {
        /* Detach the shared memory segment.  */
        shmdt( shared_memory );
        /* Deallocate the shared memory segment.  */
        shmctl( segment_id, IPC_RMID, 0 );
    }

    bool Recv( char* out, int n )
    {
        bool res = false;

        /* create a semaphore for mutual exclusion */
        sem_t* semptr = sem_open( SemaphoreName, /* name */
                                  O_CREAT,       /* create the semaphore */
                                  AccessPerms,   /* protection perms */
                                  0 );           /* initial value */
        if( semptr == ( void* ) -1 )
        {
            perror( "sem_open" );
            return false;
        }

        /* use semaphore as a mutex (lock) by waiting for writer to increment it
         */
        if( sem_trywait( semptr ) != -1 )
        { /* wait until semaphore != 0 */
            memcpy( out, shared_memory, n );
            //sem_post( semptr );
            res = true;
        }
        else
        {
            //perror( "sem_timedwait" );
        }

        sem_close( semptr );
        return res;
    }

private:
    int segment_id;
    char* shared_memory;
};

key_t shmkey = 1111;
std::unique_ptr< CSharedMemReader > led_reader =
    std::make_unique< CSharedMemReader >( shmkey );
char led_str[ 64 ] = {0};

CCMD(clearledreader)
{
	led_reader.reset(nullptr);

	if(argv.argc() > 1)
	{
		shmkey = atoi(argv[1]);
	}
	else
	{
		shmkey = 1234;
	}
	Printf( "Using shkey %d\n", shmkey );

	led_reader = std::make_unique< CSharedMemReader >( shmkey );
}

class CLogControl
{
public:
    bool Rejects( const std::string& msg )
    {
        auto extract_prefix = []( const std::string& msg ) {
            std::vector< std::string > res;
            if( msg.find( '[' ) == 0 && msg.find( "]:" ) == 2 )
            {
                res.push_back( std::string( "mach" ) + msg.substr( 1, 1 ) );
            }
            auto esc_ind = msg.find( TEXTCOLOR_ESCAPE );
            if( esc_ind != std::string::npos &&
                msg.find( TEXTCOLOR_NORMAL, esc_ind + 2 ) !=
                    std::string::npos &&
                msg.find( TEXTCOLOR_NORMAL, esc_ind + 2 ) > esc_ind + 2 )
            {
                res.push_back(
                    msg.substr( esc_ind + 2,
                                msg.find( TEXTCOLOR_NORMAL, esc_ind + 2 ) -
                                    ( esc_ind + 2 ) ) );
            }
            return res;
        };

        auto prefix = extract_prefix( msg );
        if( prefix.empty() )
        {
            return false;
        }

        bool accept_msg = true;

        for( auto p: prefix )
        {
            UpdateInfo( p );

            const auto& info = m_prefix_infos[ p ];
            if( !info.m_accept )
            {
                accept_msg = false;
            }
        }

        return !accept_msg;
    }

private:
    using TLogPrefix = std::string;
    struct CPrefixInfo
    {
        bool m_accept = true;
        std::shared_ptr< FConsoleCommand > m_toggle_cmd;
    };

    void UpdateInfo( const std::string& prefix )
    {
        if( m_prefix_infos.find( prefix ) == std::end( m_prefix_infos ) )
        {
            auto run_toggle_cmd = [prefix, this]( FCommandLine&, APlayerPawn*,
                                                  int ) {
                auto it = this->m_prefix_infos.find( prefix );
                if( it != std::end( this->m_prefix_infos ) )
                {
                    it->second.m_accept = !it->second.m_accept;
                    Printf( "Logs %s %s\n", prefix.c_str(),
                            it->second.m_accept ? "ENABLED" : "DISABLED" );
                }
            };
            auto toggle_cmd_name = // i know these are dangling pointers
                strdup( ( std::string( "togglelog-" ) + prefix ).c_str() );
            CPrefixInfo info{
                true,
                std::make_shared< FConsoleCommand >(
                    toggle_cmd_name, run_toggle_cmd ),
            };
            m_prefix_infos.insert( std::make_pair( prefix, info ) );
        }
    }

    std::map< TLogPrefix, CPrefixInfo > m_prefix_infos;
};

class CIPCHandlerThread
{
public:
    static constexpr auto PortNumber      = 9876;
    static constexpr auto MaxConnects     =    8;
    static constexpr auto BuffSize        =  512;
    static constexpr auto Host            = "localhost";

    CIPCHandlerThread()
    {
        m_thread = std::thread( [&] { Worker(); } );
    }

    ~CIPCHandlerThread()
    {
        if( m_thread.get_id() != std::thread::id{} )
        {
            {
                std::lock_guard< std::mutex > lk( m_mut );
                m_abortloop = true;
            }

            m_thread.join();
        }
    }

    void report( const char* msg, int terminate )
    {
        perror( msg );
        if( terminate )
            exit( -1 ); /* failure */
    }

    void Worker()
    {
        int fd = socket( AF_INET,     /* network versus AF_LOCAL */
                         SOCK_STREAM, /* reliable, bidirectional: TCP */
                         0 );         /* system picks underlying protocol */
        if( fd < 0 )
            report( "socket", 1 ); /* terminate */

        /* bind the server's local address in memory */
        struct sockaddr_in saddr;
        memset( &saddr, 0, sizeof( saddr ) ); /* clear the bytes */
        saddr.sin_family = AF_INET;           /* versus AF_LOCAL */
        saddr.sin_addr.s_addr =
            htonl( INADDR_ANY );              /* host-to-network endian */
        saddr.sin_port = htons( PortNumber ); /* for listening */

        int flags = fcntl( fd, F_GETFL );
        fcntl( fd, F_SETFL, flags | O_NONBLOCK );

        if( bind( fd, ( struct sockaddr* ) &saddr, sizeof( saddr ) ) < 0 )
            report( "bind", 1 ); /* terminate */

        /* listen to the socket */
        if( listen( fd, MaxConnects ) <
            0 )                    /* listen for clients, up to MaxConnects */
            report( "listen", 1 ); /* terminate */

        /* a server traditionally listens indefinitely */
        fprintf( stderr, "Listening on port %i for clients...\n", PortNumber );
        while( !m_abortloop )
        {

            struct sockaddr_in caddr;  /* client address */
            unsigned int len = sizeof( caddr ); /* address length could change */

            int client_fd = accept( fd, ( struct sockaddr* ) &caddr,
                                    &len );
            if( client_fd < 0 )
            {
                if( errno == EWOULDBLOCK )
                {
                    //printf(
                    //    "No pending connections; sleeping for one second.\n" );
                    usleep( 200000 );
                    continue;
                }
                report( "accept",
                        0 ); /* don't terminated, though there's a problem */
                continue;
            }

            fprintf( stderr, "Connected\n" );
            while( !m_abortloop )
            {
                /* read from client */
                char buffer[ 128 ];
                memset( buffer, '\0', sizeof( buffer ) );
                int count = read( client_fd, buffer, sizeof( buffer ) );
                if( count == 128 )
                {
                    std::lock_guard< std::mutex > lk( m_mut );
                    m_msg_queue.push_back( buffer );
                }

                if( count < 128 || strcmp(buffer, "QUIT") == 0 )
                {
                    fprintf( stderr, "Got quit, breaking connection\n" );
                    break;
                }
            }

            close( client_fd ); /* break connection */
        }                       /* while(1) */
    }

    const boost::optional< std::string > GetMsg()
    {
        auto now = SDL_GetTicks();
        if( now < m_last_ticks )
        {
            m_last_ticks = now;
            return boost::none;
        }

        auto ticks_prefix = []( auto delta_ticks ) {
            std::stringstream ss;
            if( delta_ticks >= 1000 )
            {
                ss << TEXTCOLOR_NONOTIFY_BEGIN << TEXTCOLOR_BOLD << std::left
                   << std::setw( 3 )
                   << std::min( delta_ticks / 1000, ( Uint32 ) 999 )
                   << TEXTCOLOR_NORMAL << " " << TEXTCOLOR_NONOTIFY_END;
            }
            else
            {
                ss << TEXTCOLOR_NONOTIFY_BEGIN << std::left << std::setw( 3 )
                   << delta_ticks << " " << TEXTCOLOR_NONOTIFY_END;
            }
            return ss.str();
        };

        std::lock_guard< std::mutex > lk( m_mut );
        if( !m_msg_queue.empty() )
        {
            auto msg = m_msg_queue.front();
            m_msg_queue.pop_front();
            if( m_log_control.Rejects( msg ) )
            {
                return boost::none;
            }

            auto delta_ticks = now - m_last_ticks;
            m_last_ticks = now;
            return ticks_prefix( delta_ticks ) + msg;
        }
        return boost::none;
    }

private:
    std::thread m_thread;
    std::mutex m_mut;
    bool m_abortloop = false;
    std::deque< std::string > m_msg_queue;
    CLogControl m_log_control;
    Uint32 m_last_ticks = 0;
};
CIPCHandlerThread ipc_handler_thread;

CVAR (String, ipc_command_host, "localhost", CVAR_ARCHIVE)

class CCommandIssuer
{
public:
    void report( const char* msg, int terminate ) const
    {
        perror( msg );
        if( terminate )
            exit( -1 ); /* failure */
    }

    int send_msg( const char* msg ) const
    {
        static constexpr auto PortNumber = 9877;
        //static constexpr auto Host = "localhost";

        /* fd for the socket */
        int sockfd = socket( AF_INET,     /* versus AF_LOCAL */
                             SOCK_STREAM, /* reliable, bidirectional */
                             0 );         /* system picks protocol (TCP) */
        if( sockfd < 0 )
        {
            report( "socket", 0 ); /* terminate */
            return false;
        }

        /* get the address of the host */
        const char* host = ipc_command_host;
        struct hostent* hptr = gethostbyname( host ); /* localhost: 127.0.0.1 */
        if( !hptr )
        {
            report( "gethostbyname", 0 ); /* is hptr NULL? */
            return false;
        }
        if( hptr->h_addrtype != AF_INET ) /* versus AF_LOCAL */
        {
            report( "bad address family", 0 );
            return false;
        }

        /* connect to the server: configure server's address 1st */
        struct sockaddr_in saddr;
        memset( &saddr, 0, sizeof( saddr ) );
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr =
            ( ( struct in_addr* ) hptr->h_addr_list[ 0 ] )->s_addr;
        saddr.sin_port = htons( PortNumber ); /* port number in big-endian */

        if( connect( sockfd, ( struct sockaddr* ) &saddr, sizeof( saddr ) ) <
            0 )
        {
            report( "connect", 0 );
            return false;
        }

        /* Write some stuff and read the echoes. */
        // puts( "Connect to server, about to write some stuff..." );
        write( sockfd, msg, strlen( msg ) );
        // puts( "Client done, about to exit..." );
        close( sockfd ); /* close the connection */
        return true;
    }

    bool SendIPCMessage( const std::string& msg ) const
    {
        return send_msg( msg.c_str() );
    }
};

CCMD(sendcmd)
{
    std::stringstream ss;
    for( int i = 1; i < argv.argc(); i++ )
    {
        ss << argv[i] << ' ';
    }
    auto cmd = ss.str();

    printf("sending cmd: %s\n", cmd.c_str());
    CCommandIssuer{}.SendIPCMessage( cmd );
}

AutoMap::Color &AutoMap::Color::operator=(int rgb)
{
	color = rgb;
	palcolor = ColorMatcher.Pick(RPART(rgb), GPART(rgb), BPART(rgb));
	return *this;
}

AutoMap AM_Main;
AutoMap AM_Overlay;

enum
{
	AMO_Off,
	AMO_On,
	AMO_Both
};
enum
{
	AMR_Off,
	AMR_On,
	AMR_Overlay
};

unsigned automap = 0;
CVAR (Bool, am_cheat, false, CVAR_ARCHIVE)
CVAR (Int, am_rotate, 0, CVAR_ARCHIVE)
CVAR (Bool, am_overlaytextured, false, CVAR_ARCHIVE)
CVAR (Bool, am_drawtexturedwalls, true, CVAR_ARCHIVE)
CVAR (Bool, am_drawfloors, false, CVAR_ARCHIVE)
CVAR (Int, am_overlay, 0, CVAR_ARCHIVE)
CVAR (Bool, am_pause, true, CVAR_ARCHIVE)
CVAR (Bool, am_showratios, false, CVAR_ARCHIVE)
bool am_needsrecalc = false;

void AM_ChangeResolution()
{
	if(StatusBar == NULL)
	{
		am_needsrecalc = true;
		return;
	}

	unsigned int y = statusbary1;
	unsigned int height = statusbary2 - statusbary1;

	AM_Main.CalculateDimensions(0, y, screenWidth, height);
	AM_Overlay.CalculateDimensions(viewscreenx, viewscreeny, viewwidth, viewheight);
}

void AM_CheckKeys()
{
	if(control[ConsolePlayer].ambuttonstate[bt_zoomin])
	{
		AM_Overlay.SetScale(FRACUNIT*135/128, true);
		AM_Main.SetScale(FRACUNIT*135/128, true);
	}
	if(control[ConsolePlayer].ambuttonstate[bt_zoomout])
	{
		AM_Overlay.SetScale(FRACUNIT*122/128, true);
		AM_Main.SetScale(FRACUNIT*122/128, true);
	}

	if(am_pause)
	{
		const fixed PAN_AMOUNT = FixedDiv(FRACUNIT*10, AM_Main.GetScreenScale());
		const fixed PAN_ANALOG_MULTIPLIER = PAN_AMOUNT/100;
		fixed panx = 0, pany = 0;

		//if(control[ConsolePlayer].ambuttonstate[bt_panleft])
			panx += PAN_AMOUNT;
		if(control[ConsolePlayer].ambuttonstate[bt_panright])
			panx -= PAN_AMOUNT;
		//if(control[ConsolePlayer].ambuttonstate[bt_panup])
			pany += PAN_AMOUNT;
		if(control[ConsolePlayer].ambuttonstate[bt_pandown])
			pany -= PAN_AMOUNT;

		if(me_marker && !control[ConsolePlayer].buttonstate[bt_run])
		{
			panx = -panx;
			pany = -pany;
		}

		if(control[ConsolePlayer].controlpanx != 0)
			panx += control[ConsolePlayer].controlpanx * (PAN_ANALOG_MULTIPLIER * (panxadjustment+1));

		if(control[ConsolePlayer].controlpany != 0)
			pany += control[ConsolePlayer].controlpany * (PAN_ANALOG_MULTIPLIER * (panxadjustment+1));

		if(me_marker && !control[ConsolePlayer].buttonstate[bt_run])
		{
			AActor *ob = players[0].mo;

			AM_Main.SetPanning(panx, pany, true);

			fixed basex = ob->x;
			fixed basey = ob->y;

			ob->x = basex + panx;
			ob->y = basey + pany;
			if(!TryMove (ob))
			{
				ob->x = basex;
				ob->y = basey;
			}
		}
		else
		{
			AM_Main.SetPanning(panx, pany, true);
		}
	}

	if(control[ConsolePlayer].ambuttonstate[bt_amtoggleconsole] && !control[ConsolePlayer].ambuttonheld[bt_amtoggleconsole])
		C_ToggleConsole();
}

void AM_UpdateFlags()
{
	// Release pause if we appear to have unset am_pause
	if(!am_pause && (Paused&2)) Paused &= ~2;

	unsigned int flags = 0;
	unsigned int ovFlags = AutoMap::AMF_Overlay;

	if(am_rotate == AMR_On) flags |= AutoMap::AMF_Rotate;

	// Overlay only flags
	ovFlags |= flags;
	if(am_rotate == AMR_Overlay) ovFlags |= AutoMap::AMF_Rotate;
	if(am_overlaytextured) ovFlags |= AutoMap::AMF_DrawTexturedWalls;

	// Full only flags
	if(am_showratios) flags |= AutoMap::AMF_DispRatios;
	if(am_drawfloors) flags |= AutoMap::AMF_DrawFloor;
	if(am_drawtexturedwalls) flags |= AutoMap::AMF_DrawTexturedWalls;

	AM_Main.SetFlags(~flags, false);
	AM_Overlay.SetFlags(~ovFlags, false);
	AM_Main.SetFlags(flags|AutoMap::AMF_DispInfo|AutoMap::AMF_ShowThings, true);
	AM_Overlay.SetFlags(ovFlags, true);
}

void AM_Toggle()
{
	++automap;
	if(automap == AMA_Overlay && am_overlay == AMO_Off)
		++automap;
	else if(automap > AMA_Normal || (automap == AMA_Normal && am_overlay == AMO_On))
	{
		automap = AMA_Off;
		AM_Main.SetPanning(0, 0, false);
		DrawPlayScreen();
	}

	if(am_pause)
	{
		if(automap == AMA_Normal)
			Paused |= 2;
		else
			Paused &= ~2;
	}
}

// Culling table
static const struct
{
	const struct
	{
		const unsigned short Min, Max;
	} X, Y;
} AM_MinMax[4] = {
	{ {3, 1}, {0, 2} },
	{ {2, 0}, {3, 1} },
	{ {1, 3}, {2, 0} },
	{ {0, 2}, {1, 3} }
};

// #FF9200
struct AMVectorPoint
{
	fixed X, Y;
};
static const AMVectorPoint AM_Arrow[] =
{
	{0, -FRACUNIT/2},
	{FRACUNIT/2, 0},
	{FRACUNIT/4, 0},
	{FRACUNIT/4, FRACUNIT*7/16},
	{-FRACUNIT/4, FRACUNIT*7/16},
	{-FRACUNIT/4, 0},
	{-FRACUNIT/2, 0},
	{0, -FRACUNIT/2},
};

AutoMap::AutoMap(unsigned int flags) :
	fullRefresh(true), amFlags(flags),
	ampanx(0), ampany(0)
{
	amangle = 0;
	minmaxSel = 0;
	amsin = 0;
	amcos = FRACUNIT;
	absscale = FRACUNIT;
	rottable[0][0] = 1.0;
	rottable[0][1] = 0.0;
	rottable[1][0] = 1.0;
	rottable[1][1] = 1.0;
}

AutoMap::~AutoMap()
{
}

void AutoMap::CalculateDimensions(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	fullRefresh = true;
	amsizex = width;
	amsizey = height;
	amx = x;
	amy = y;

	// Since the simple polygon fill function seems to be off by one in the y
	// direction, lets shift this up!
	--amy;

	// TODO: Find a better spot for this
	ArrowColor = gameinfo.automap.YourColor;
	BackgroundColor = gameinfo.automap.Background;
	FloorColor = gameinfo.automap.FloorColor;
	WallColor = gameinfo.automap.WallColor;
	DoorColor = gameinfo.automap.DoorColor;
}

// Sutherland–Hodgman algorithm
void AutoMap::ClipTile(TArray<FVector2> &points) const
{
	for(int i = 0;i < 4;++i)
	{
		TArray<FVector2> input(points);
		points.Clear();
		FVector2 *s = &input[0];
		for(unsigned j = input.Size();j-- > 0;)
		{
			FVector2 &e = input[j];

			bool Ein, Sin;
			switch(i)
			{
				case 0: // Left
					Ein = e.X > amx;
					Sin = s->X > amx;
					break;
				case 1: // Top
					Ein = e.Y > amy;
					Sin = s->Y > amy;
					break;
				case 2: // Right
					Ein = e.X < amx+amsizex;
					Sin = s->X < amx+amsizex;
					break;
				case 3: // Bottom
					Ein = e.Y < amy+amsizey;
					Sin = s->Y < amy+amsizey;
					break;
			}
			if(Ein)
			{
				if(!Sin)
					points.Push(GetClipIntersection(e, *s, i));
				points.Push(e);
			}
			else if(Sin)
				points.Push(GetClipIntersection(e, *s, i));
			s = &e;
		}
	}
}

struct AMPWall
{
public:
	TArray<FVector2> points;
	FTextureID texid;
	float shiftx, shifty;
};
void AutoMap::Draw()
{
	TArray<AMPWall> pwalls;
	TArray<FVector2> points;
	const unsigned int mapwidth = map->GetHeader().width;
	const unsigned int mapheight = map->GetHeader().height;

	const fixed scale = GetScreenScale();

	if(!(amFlags & AMF_Overlay))
		screen->Clear(amx, amy+1, amx+amsizex, amy+amsizey+1, BackgroundColor.palcolor, BackgroundColor.color);

	const fixed playerx = players[0].mo->x;
	const fixed playery = players[0].mo->y;

	if(fullRefresh || amangle != ((amFlags & AMF_Rotate) ? players[0].mo->angle-ANGLE_90 : 0))
	{
		amangle = (amFlags & AMF_Rotate) ? players[0].mo->angle-ANGLE_90 : 0;
		minmaxSel = amangle/ANGLE_90;
		amsin = finesine[amangle>>ANGLETOFINESHIFT];
		amcos = finecosine[amangle>>ANGLETOFINESHIFT];

		// For rotating the tiles, this table includes the point offsets based on the current scale
		rottable[0][0] = FIXED2FLOAT(FixedMul(scale, amcos)); rottable[0][1] = FIXED2FLOAT(FixedMul(scale, amsin));
		rottable[1][0] = FIXED2FLOAT(FixedMul(scale, amcos) - FixedMul(scale, amsin)); rottable[1][1] = FIXED2FLOAT(FixedMul(scale, amsin) + FixedMul(scale, amcos));

		fullRefresh = false;
	}

	if( led_reader )
	{
		led_reader->Recv( led_str, 40 );
	}

	auto msg = ipc_handler_thread.GetMsg();
	if(msg)
	{
		Printf("%s\n", (*msg).c_str());
		printf("%s\n", (*msg).c_str());
	}

	const fixed pany = FixedMul(ampany, amcos) - FixedMul(ampanx, amsin);
	const fixed panx = FixedMul(ampany, amsin) + FixedMul(ampanx, amcos);
	const fixed ofsx = playerx - panx;
	const fixed ofsy = playery - pany;

	const double originx = (amx+amsizex/2) - (FIXED2FLOAT(FixedMul(FixedMul(scale, ofsx&0xFFFF), amcos) - FixedMul(FixedMul(scale, ofsy&0xFFFF), amsin)));
	const double originy = (amy+amsizey/2) - (FIXED2FLOAT(FixedMul(FixedMul(scale, ofsx&0xFFFF), amsin) + FixedMul(FixedMul(scale, ofsy&0xFFFF), amcos)));
	const double origTexScale = FIXED2FLOAT(scale>>6);

	for(unsigned int my = 0;my < mapheight;++my)
	{
		MapSpot spot = map->GetSpot(0, my, 0);
		for(unsigned int mx = 0;mx < mapwidth;++mx, ++spot)
		{
			if(!((spot->amFlags & AM_Visible) || am_cheat || gamestate.fullmap) ||
				((amFlags & AMF_Overlay) && (spot->amFlags & AM_DontOverlay)))
				continue;

			if(TransformTile(spot, FixedMul((mx<<FRACBITS)-ofsx, scale), FixedMul((my<<FRACBITS)-ofsy, scale), points))
			{
				double texScale = origTexScale;

				FTexture *tex;
				Color *color = NULL;
				int brightness;
				if(my == 0 && mx < 5)
				{
					// #RRGGBB
					FString cpt_str(&led_str[mx * 8], 7);
					FTextureID texid = TexMan.GetTexture(cpt_str, FTexture::TEX_Flat);
					if( texid.isValid() )
						tex = TexMan(texid);
					else
						tex = NULL;
				}
				else if(spot->tile && !spot->pushAmount && !spot->pushReceptor)
				{
					brightness = 256;
					if((amFlags & AMF_DrawTexturedWalls))
					{
						if(spot->tile->overhead.isValid())
							tex = TexMan(spot->tile->overhead);
						else if(spot->tile->offsetHorizontal)
							tex = TexMan(spot->texture[MapTile::North]);
						else
							tex = TexMan(spot->texture[MapTile::East]);
					}
					else
					{
						tex = NULL;
						if(spot->tile->offsetHorizontal || spot->tile->offsetVertical)
							color = &DoorColor;
						else
							color = &WallColor;
					}
				}
				else if(spot->sector && !(amFlags & AMF_Overlay))
				{
					if(spot->sector->overhead.isValid())
					{
						brightness = 256;
						tex = TexMan(spot->sector->overhead);
					}
					else if(amFlags & AMF_DrawFloor)
					{
						brightness = 128;
						tex = TexMan(spot->sector->texture[MapSector::Floor]);
					}
					else
					{
						brightness = 256;
						tex = NULL;
						if(FloorColor.color != BackgroundColor.color)
							color = &FloorColor;
					}
				}
				else
				{
					tex = NULL;
				}

				if(tex)
				{
					// As a special case, since Noah's Ark stores the Automap
					// graphics in the TILE8, we need to override the scaling.
					if(tex->UseType == FTexture::TEX_FontChar)
						texScale *= 8;
					screen->FillSimplePoly(tex, &points[0], points.Size(), originx, originy, texScale, texScale, ~amangle, &NormalLight, brightness);
				}
				else if(color)
					screen->FillSimplePoly(NULL, &points[0], points.Size(), originx, originy, texScale, texScale, ~amangle, &NormalLight, brightness, color->palcolor, color->color);
			}

			// We need to check this even if the origin tile isn't visible since
			// the destination spot may be!
			if(spot->pushAmount)
			{
				fixed tx = (mx<<FRACBITS), ty = my<<FRACBITS;
				switch(spot->pushDirection)
				{
					default:
					case MapTile::East:
						tx += (spot->pushAmount<<10);
						break;
					case MapTile::West:
						tx -= (spot->pushAmount<<10);
						break;
					case MapTile::South:
						ty += (spot->pushAmount<<10);
						break;
					case MapTile::North:
						ty -= (spot->pushAmount<<10);
						break;
				}
				if(TransformTile(spot, FixedMul(tx-ofsx, scale), FixedMul(ty-ofsy, scale), points))
				{
					AMPWall pwall;
					pwall.points = points;
					pwall.texid = spot->tile->overhead.isValid() ? spot->tile->overhead : spot->texture[0];
					pwall.shiftx = (float)(FIXED2FLOAT(FixedMul(FixedMul(scale, tx&0xFFFF), amcos) - FixedMul(FixedMul(scale, ty&0xFFFF), amsin)));
					pwall.shifty = (float)(FIXED2FLOAT(FixedMul(FixedMul(scale, tx&0xFFFF), amsin) + FixedMul(FixedMul(scale, ty&0xFFFF), amcos)));
					pwalls.Push(pwall);
				}
			}
		}
	}

	for(unsigned int pw = pwalls.Size();pw-- > 0;)
	{
		AMPWall &pwall = pwalls[pw];
		if((amFlags & AMF_DrawTexturedWalls))
		{
			double texScale = origTexScale;
			FTexture *tex = TexMan(pwall.texid);
			if(tex)
			{
				// Noah's ark TILE8
				if(tex->UseType == FTexture::TEX_FontChar)
					texScale *= 8;
				screen->FillSimplePoly(tex, &pwall.points[0], pwall.points.Size(), originx + pwall.shiftx, originy + pwall.shifty, texScale, texScale, ~amangle, &NormalLight, 256);
			}
		}
		else
			screen->FillSimplePoly(NULL, &pwall.points[0], pwall.points.Size(), originx + pwall.shiftx, originy + pwall.shifty, origTexScale, origTexScale, ~amangle, &NormalLight, 256, WallColor.palcolor, WallColor.color);
	}

	DrawVector(AM_Arrow, 8, FixedMul(playerx - ofsx, scale), FixedMul(playery - ofsy, scale), scale, (amFlags & AMF_Rotate) ? 0 : ANGLE_90-players[0].mo->angle, ArrowColor);

	if((amFlags & AMF_ShowThings) && (am_cheat || gamestate.fullmap))
	{
		for(AActor::Iterator iter = AActor::GetIterator();iter.Next();)
		{
			if(am_cheat || (gamestate.fullmap && (iter->flags & FL_PLOTONAUTOMAP)))
				DrawActor(iter, FixedMul(iter->x - ofsx, scale), FixedMul(iter->y - ofsy, scale), scale);
		}
	}

	//DrawStats();
}

void AutoMap::DrawActor(AActor *actor, fixed x, fixed y, fixed scale)
{
	{
		fixed tmp = ((FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1)))>>FRACBITS) + amx;
		y = ((FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1)))>>FRACBITS) + amy;
		x = tmp;

		int adiameter = FixedMul(actor->radius, scale)>>(FRACBITS-1);
		int aheight = scale>>FRACBITS;

		// Cull out actors that are obviously out of bounds
		if(x + adiameter < amx || x - adiameter >= amx+amsizex || y + aheight < amy || y - aheight >= amy+amsizey)
			return;
	}

	bool flip;
	FTexture *tex;
	if(actor->overheadIcon.isValid())
	{
		tex = actor->sprite != SPR_NONE ? TexMan(actor->overheadIcon) : NULL;
		flip = false;
	}
	else
		tex = R_GetAMSprite(actor, amangle, flip);

	if(!tex)
		return;

	if(tex->UseType != FTexture::TEX_FontChar)
	{
		double width = tex->GetScaledWidthDouble()*FIXED2FLOAT(scale>>6);
		double height = tex->GetScaledHeightDouble()*FIXED2FLOAT(scale>>6);
		screen->DrawTexture(tex, x, y,
			DTA_DestWidthF, width,
			DTA_DestHeightF, height,
			DTA_ClipLeft, amx,
			DTA_ClipRight, amx+amsizex,
			DTA_ClipTop, amy,
			DTA_ClipBottom, amy+amsizey,
			DTA_FlipX, flip,
			TAG_DONE);
	}
	else
	{
		screen->DrawTexture(tex, x - (scale>>(FRACBITS+1)), y - (scale>>(FRACBITS+1)),
			DTA_DestWidthF, FIXED2FLOAT(scale),
			DTA_DestHeightF, FIXED2FLOAT(scale),
			DTA_ClipLeft, amx,
			DTA_ClipRight, amx+amsizex,
			DTA_ClipTop, amy,
			DTA_ClipBottom, amy+amsizey,
			DTA_FlipX, flip,
			TAG_DONE);
	}
}

void AutoMap::DrawClippedLine(int x0, int y0, int x1, int y1, int palcolor, uint32 realcolor) const
{
	// Let us make an assumption that point 0 is always left of point 1
	if(x0 > x1)
	{
		swapvalues(x0, x1);
		swapvalues(y0, y1);
	}

	const int dx = x1 - x0, dy = y1 - y0;
	int ymin = MIN(y0, y1);
	int ymax = MAX(y0, y1);
	const bool inc = ymax == y0;
	bool clipped = false;

	// There will inevitably be precision issues so we may need to run the
	// clipper a few times.
	do
	{
		// Trivial culling
		if(x1 < amx || ymax < amy || x0 >= amx+amsizex || ymin >= amy+amsizey)
			return;

		if(x0 < amx) // Clip left
		{
			clipped = true;
			y0 += dy*(amx-x0)/dx;
			x0 = amx;
		}
		if(x1 >= amx+amsizex) // Clip right
		{
			clipped = true;
			y1 += dy*(amx+amsizex-1-x1)/dx;
			x1 = amx+amsizex-1;
		}
		if(ymin < amy) // Clip top
		{
			clipped = true;
			if(inc)
			{
				x1 += dx*(amy-y1)/dy;
				y1 = amy;
			}
			else
			{
				x0 += dx*(amy-y0)/dy;
				y0 = amy;
			}
		}
		if(ymax >= amy+amsizey) // Clip bottom
		{
			clipped = true;
			if(inc)
			{
				x0 += dx*(amy+amsizey-1-y0)/dy;
				y0 = amy+amsizey-1;
			}
			else
			{
				x1 += dx*(amy+amsizey-1-y1)/dy;
				y1 = amy+amsizey-1;
			}
		}

		if(!clipped)
			break;
		clipped = false;

		// Fix ymin/max for the next iteration
		if(inc)
		{
			ymin = y1;
			ymax = y0;
		}
		else
		{
			ymin = y0;
			ymax = y1;
		}
	}
	while(true);

	screen->DrawLine(x0, y0+1, x1, y1+1, palcolor, realcolor);
}

void AutoMap::DrawStats() const
{
	if(!(amFlags & (AMF_DispInfo|AMF_DispRatios)))
		return;

	FString statString;
	unsigned int infHeight = 0;

	if(amFlags & AMF_DispInfo)
	{
		infHeight = SmallFont->GetHeight()+2;
		screen->Dim(GPalette.BlackIndex, 0.5f, 0, 0, screenWidth, infHeight*CleanYfac);

		screen->DrawText(SmallFont, gameinfo.automap.FontColor, 2*CleanXfac, CleanYfac, levelInfo->GetName(map),
			DTA_CleanNoMove, true,
			TAG_DONE);

		unsigned int seconds = gamestate.TimeCount/70;
		statString.Format("%02d:%02d:%02d", seconds/3600, (seconds%3600)/60, seconds%60);
		screen->DrawText(SmallFont, gameinfo.automap.FontColor,
			screenWidth - (SmallFont->GetCharWidth('0')*6 + SmallFont->GetCharWidth(':')*2 + 2)*CleanXfac, CleanYfac,
			statString,
			DTA_CleanNoMove, true,
			TAG_DONE);
	}

	if(amFlags & AMF_DispRatios)
	{
		statString.Format("K: %d/%d\nS: %d/%d\nT: %d/%d",
			gamestate.killcount, gamestate.killtotal,
			gamestate.secretcount, gamestate.secrettotal,
			gamestate.treasurecount, gamestate.treasuretotal);

		word sw, sh;
		VW_MeasurePropString(SmallFont, statString, sw, sh);
		screen->Dim(GPalette.BlackIndex, 0.5f, 0, infHeight*CleanYfac, (sw+3)*CleanXfac, (sh+2)*CleanYfac);

		screen->DrawText(SmallFont, gameinfo.automap.FontColor, 2*CleanXfac, infHeight*CleanYfac, statString,
			DTA_CleanNoMove, true,
			TAG_DONE);
	}
}

void AutoMap::DrawVector(const AMVectorPoint *points, unsigned int numPoints, fixed x, fixed y, fixed scale, angle_t angle, const Color &c) const
{
	int x1, y1, x2, y2;

	fixed tmp = ((FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1)))>>FRACBITS) + amx;
	y = ((FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1)))>>FRACBITS) + amy;
	x = tmp;

	x1 = FixedMul(points[numPoints-1].X, scale)>>FRACBITS;
	y1 = FixedMul(points[numPoints-1].Y, scale)>>FRACBITS;
	if(angle)
	{
		const fixed rsin = finesine[angle>>ANGLETOFINESHIFT];
		const fixed rcos = finecosine[angle>>ANGLETOFINESHIFT];
		int tmp;
		tmp = FixedMul(x1, rcos) - FixedMul(y1, rsin);
		y1 = FixedMul(x1, rsin) + FixedMul(y1, rcos);
		x1 = tmp;

		for(unsigned int i = numPoints-1;i-- > 0;x1 = x2, y1 = y2)
		{
			x2 = FixedMul(points[i].X, scale)>>FRACBITS;
			y2 = FixedMul(points[i].Y, scale)>>FRACBITS;

			tmp = FixedMul(x2, rcos) - FixedMul(y2, rsin);
			y2 = FixedMul(x2, rsin) + FixedMul(y2, rcos);
			x2 = tmp;

			DrawClippedLine(x + x1, y + y1, x + x2, y + y2, c.palcolor, c.color);
		}
	}
	else
	{
		for(unsigned int i = numPoints-1;i-- > 0;x1 = x2, y1 = y2)
		{
			x2 = FixedMul(points[i].X, scale)>>FRACBITS;
			y2 = FixedMul(points[i].Y, scale)>>FRACBITS;

			DrawClippedLine(x + x1, y + y1, x + x2, y + y2, c.palcolor, c.color);
		}
	}
}

FVector2 AutoMap::GetClipIntersection(const FVector2 &p1, const FVector2 &p2, unsigned edge) const
{
	// If we are clipping a vertical line, it should be because our angle is
	// 90 degrees and it's clip against the top or bottom
	if((edge&1) == 1 && ((amangle>>ANGLETOFINESHIFT)&(ANG90-1)) == 0)
	{
		if(edge == 1)
			return FVector2(p1.X, amy);
		return FVector2(p1.X, amy+amsizey);
	}
	else
	{
		const float A = p2.Y - p1.Y;
		const float B = p1.X - p2.X;
		const float C = A*p1.X + B*p1.Y;
		switch(edge)
		{
			default:
			case 0: // Left
				return FVector2(amx, (C - A*amx)/B);
			case 1: // Top
				return FVector2((C - B*amy)/A, amy);
			case 2: // Right
				return FVector2(amx+amsizex, (C - A*(amx+amsizex))/B);
			case 3: // Bottom
				return FVector2((C - B*(amy+amsizey))/A, amy+amsizey);
		}
	}
}

// Returns size of a tile in pixels
fixed AutoMap::GetScreenScale() const
{
	// Some magic, min scale is approximately small enough so that a rotated automap will fit on screen (22/32 ~ 1/sqrt(2))
	const fixed minscale = ((screenHeight*22)<<(FRACBITS-5))/map->GetHeader().height;
	return minscale + FixedMul(absscale, (screenHeight<<(FRACBITS-3)) - minscale);
}

void AutoMap::SetFlags(unsigned int flags, bool set)
{
	if(set)
		amFlags |= flags;
	else
		amFlags &= ~flags;
}

void AutoMap::SetPanning(fixed x, fixed y, bool relative)
{
	if(relative)
	{
		// TODO: Make panning absolute instead of relative to the player so this isn't weird
		fixed posx, posy, maxx, maxy;
		if(amangle)
		{
			const int sizex = map->GetHeader().width;
			const int sizey = map->GetHeader().height;
			maxx = abs(sizex*amcos) + abs(sizey*amsin);
			maxy = abs(sizex*amsin) + abs(sizey*amcos);
			posx = players[0].mo->x - (sizex<<(FRACBITS-1));
			posy = players[0].mo->y - (sizey<<(FRACBITS-1));
			fixed tmp = (FixedMul(posx, amcos) - FixedMul(posy, amsin)) + (maxx>>1);
			posy = (FixedMul(posy, amcos) + FixedMul(posx, amsin)) + (maxy>>1);
			posx = tmp;
		}
		else
		{
			maxx = map->GetHeader().width<<FRACBITS;
			maxy = map->GetHeader().height<<FRACBITS;
			posx = players[0].mo->x;
			posy = players[0].mo->y;
		}
		ampanx = clamp<fixed>(ampanx+x, posx - maxx, posx);
		ampany = clamp<fixed>(ampany+y, posy - maxy, posy);
	}
	else
	{
		ampanx = x;
		ampany = y;
	}
}

void AutoMap::SetScale(fixed scale, bool relative)
{
	if(relative)
		absscale = FixedMul(absscale, scale);
	else
		absscale = scale;
	absscale = clamp<fixed>(absscale, FRACUNIT/50, FRACUNIT);

	fullRefresh = true;
}

bool AutoMap::TransformTile(MapSpot spot, fixed x, fixed y, TArray<FVector2> &points) const
{
	fixed rotx = FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1));
	fixed roty = FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1));
	double x1 = amx + FIXED2FLOAT(rotx);
	double y1 = amy + FIXED2FLOAT(roty);
	points.Resize(4);
	points[0] = FVector2(x1, y1);
	points[1] = FVector2(x1 + rottable[0][0], y1 + rottable[0][1]);
	points[2] = FVector2(x1 + rottable[1][0], y1 + rottable[1][1]);
	points[3] = FVector2(x1 - rottable[0][1], y1 + rottable[0][0]);


	const float xmin = points[AM_MinMax[minmaxSel].X.Min].X;
	const float xmax = points[AM_MinMax[minmaxSel].X.Max].X;
	const float ymin = points[AM_MinMax[minmaxSel].Y.Min].Y;
	const float ymax = points[AM_MinMax[minmaxSel].Y.Max].Y;

	// Cull out tiles which never touch the automap area
	if((xmax < amx || xmin > amx+amsizex) || (ymax < amy || ymin > amy+amsizey))
		return false;

	// If the tile is partially in the automap area, clip
	if((ymax > amy+amsizey || ymin < amy) || (xmax > amx+amsizex || xmin < amx))
		ClipTile(points);
	return true;
}

void BasicOverhead()
{
	MapEdit::AdjustGameMap adjustGameMap;

	if(am_needsrecalc)
		AM_ChangeResolution();

	switch(automap)
	{
		case AMA_Overlay:
			AM_Overlay.Draw();
			break;
		case AMA_Normal:
		{
			int oldviewsize = viewsize;
			viewsize = 20;
			DrawPlayScreen();
			viewsize = oldviewsize;
			AM_Main.Draw();
			break;
		}
		default: break;
	}
}
