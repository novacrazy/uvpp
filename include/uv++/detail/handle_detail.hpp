//
// Created by Aaron on 7/27/2016.
//

#ifndef UV_HANDLE_DETAIL_HPP
#define UV_HANDLE_DETAIL_HPP

//For strsignal if available
#include <csignal>
#include <cstring>

#include <string>
#include <algorithm>

#if !defined( UV_NO_HAS_STRSIGNAL ) && defined( __MINGW32__ )
#define UV_NO_HAS_STRSIGNAL
#endif

namespace uv {
    namespace detail {
        /*
         * So this is here because MinGW doesn't have a working strsignal implementation for some reason, so on MinGW
         * I have to manually list every single possible signal because I don't know what is and isn't on every system.
         *
         * If it turns out other systems also don't have strsignal, they can be easily added here.
         *
         * The current list was taken from:
         *  https://en.wikipedia.org/wiki/Unix_signal
         * and
         *  https://github.com/msysgit/msys/blob/master/libiberty/strsignal.c
         * and
         *  ftp://ftp.stu.edu.tw/BSD/NetBSD/NetBSD/NetBSD-release-6/src/gnu/dist/gdb6/gdb/signals/signals.c
         *
         * If I'm missing any, feel free to open a pull request or just an issue.
         * */
        std::string signame( int signum ) {
#ifdef UV_NO_HAS_STRSIGNAL
            const char *signame;
            switch( signum ) {
                /*
                 * List of all signals so far:
                 *  SIGABRT SIGALRM SIGBUS SIGCHLD SIGCONT SIGFPE SIGHUP SIGILL SIGINT SIGKILL SIGPIPE SIGPOLL SIGPROF
                 *  SIGQUIT SIGSEGV SIGSTOP SIGSYS SIGTERM SIGTRAP SIGTSTP SIGTTIN SIGTTOU SIGUSR1 SIGUSR2 SIGURG
                 *  SIGVTALRM SIGXCPU SIGXFSZ SIGEMT SIGINFO SIGPWR SIGLOST SIGWINCH SIGIOT SIGCLD SIGIO SIGWIND
                 *  SIGPHONE SIGWAITING SIGLWP SIGDANGER SIGGRANT SIGRETRACT SIGMSG SIGSOUND SIGSAK SIGPRIO SIG33 SIG34
                 *  SIG35 SIG36 SIG37 SIG38 SIG39 SIG40 SIG41 SIG42 SIG43 SIG44 SIG45 SIG46 SIG47 SIG48 SIG49 SIG50
                 *  SIG51 SIG52 SIG53 SIG54 SIG55 SIG56 SIG57 SIG58 SIG59 SIG60 SIG61 SIG62 SIG63 SIGCANCEL SIG32 SIG64
                 *  SIG65 SIG66 SIG67 SIG68 SIG69 SIG70 SIG71 SIG72 SIG73 SIG74 SIG75 SIG76 SIG77 SIG78 SIG79 SIG80
                 *  SIG81 SIG82 SIG83 SIG84 SIG85 SIG86 SIG87 SIG88 SIG89 SIG90 SIG91 SIG92 SIG93 SIG94 SIG95 SIG96
                 *  SIG97 SIG98 SIG99 SIG100 SIG101 SIG102 SIG103 SIG104 SIG105 SIG106 SIG107 SIG108 SIG109 SIG110
                 *  SIG111 SIG112 SIG113 SIG114 SIG115 SIG116 SIG117 SIG118 SIG119 SIG120 SIG121 SIG122 SIG123 SIG124
                 *  SIG125 SIG126 SIG127 EXC_BAD_ACCESS EXC_BAD_INSTRUCTION EXC_ARITHMETIC EXC_EMULATION EXC_SOFTWARE
                 *  EXC_BREAKPOINT
                 * */

#define UV_SIGCASE( SIGNAL ) case SIGNAL: signame = #SIGNAL; break;

#ifdef SIGABRT
                UV_SIGCASE( SIGABRT )
#endif
#ifdef SIGALRM
                UV_SIGCASE( SIGALRM )
#endif
#ifdef SIGBUS
                UV_SIGCASE( SIGBUS )
#endif
#ifdef SIGCHLD
                UV_SIGCASE( SIGCHLD )
#endif
#ifdef SIGCONT
                UV_SIGCASE( SIGCONT )
#endif
#ifdef SIGFPE
                UV_SIGCASE( SIGFPE )
#endif
#ifdef SIGHUP
                UV_SIGCASE( SIGHUP )
#endif
#ifdef SIGILL
                UV_SIGCASE( SIGILL )
#endif
#ifdef SIGINT
                UV_SIGCASE( SIGINT )
#endif
#ifdef SIGKILL
                UV_SIGCASE( SIGKILL )
#endif
#ifdef SIGPIPE
                UV_SIGCASE( SIGPIPE )
#endif
#ifdef SIGPOLL
                UV_SIGCASE( SIGPOLL )
#endif
#ifdef SIGPROF
                UV_SIGCASE( SIGPROF )
#endif
#ifdef SIGQUIT
                UV_SIGCASE( SIGQUIT )
#endif
#ifdef SIGSEGV
                UV_SIGCASE( SIGSEGV )
#endif
#ifdef SIGSTOP
                UV_SIGCASE( SIGSTOP )
#endif
#ifdef SIGSYS
                UV_SIGCASE( SIGSYS )
#endif
#ifdef SIGTERM
                UV_SIGCASE( SIGTERM )
#endif
#ifdef SIGTRAP
                UV_SIGCASE( SIGTRAP )
#endif
#ifdef SIGTSTP
                UV_SIGCASE( SIGTSTP )
#endif
#ifdef SIGTTIN
                UV_SIGCASE( SIGTTIN )
#endif
#ifdef SIGTTOU
                UV_SIGCASE( SIGTTOU )
#endif
#ifdef SIGUSR1
                UV_SIGCASE( SIGUSR1 )
#endif
#ifdef SIGUSR2
                UV_SIGCASE( SIGUSR2 )
#endif
#ifdef SIGURG
                UV_SIGCASE( SIGURG )
#endif
#ifdef SIGVTALRM
                UV_SIGCASE( SIGVTALRM )
#endif
#ifdef SIGXCPU
                UV_SIGCASE( SIGXCPU )
#endif
#ifdef SIGXFSZ
                UV_SIGCASE( SIGXFSZ )
#endif
#ifdef SIGEMT
                UV_SIGCASE( SIGEMT )
#endif
#ifdef SIGINFO
                UV_SIGCASE( SIGINFO )
#endif
#ifdef SIGPWR
                UV_SIGCASE( SIGPWR )
#endif
#ifdef SIGLOST
                UV_SIGCASE( SIGLOST )
#endif
#ifdef SIGWINCH
                UV_SIGCASE( SIGWINCH )
#endif
#ifdef SIGIOT
                UV_SIGCASE( SIGIOT )
#endif
#ifdef SIGCLD
                UV_SIGCASE( SIGCLD )
#endif
#ifdef SIGIO
                UV_SIGCASE( SIGIO )
#endif
#ifdef SIGWIND
                UV_SIGCASE( SIGWIND )
#endif
#ifdef SIGPHONE
                UV_SIGCASE( SIGPHONE )
#endif
#ifdef SIGWAITING
                UV_SIGCASE( SIGWAITING )
#endif
#ifdef SIGLWP
                UV_SIGCASE( SIGLWP )
#endif
#ifdef SIGDANGER
                UV_SIGCASE( SIGDANGER )
#endif
#ifdef SIGGRANT
                UV_SIGCASE( SIGGRANT )
#endif
#ifdef SIGRETRACT
                UV_SIGCASE( SIGRETRACT )
#endif
#ifdef SIGMSG
                UV_SIGCASE( SIGMSG )
#endif
#ifdef SIGSOUND
                UV_SIGCASE( SIGSOUND )
#endif
#ifdef SIGSAK
                UV_SIGCASE( SIGSAK )
#endif
#ifdef SIGPRIO
                UV_SIGCASE( SIGPRIO )
#endif
#ifdef SIG33
                UV_SIGCASE( SIG33 )
#endif
#ifdef SIG34
                UV_SIGCASE( SIG34 )
#endif
#ifdef SIG35
                UV_SIGCASE( SIG35 )
#endif
#ifdef SIG36
                UV_SIGCASE( SIG36 )
#endif
#ifdef SIG37
                UV_SIGCASE( SIG37 )
#endif
#ifdef SIG38
                UV_SIGCASE( SIG38 )
#endif
#ifdef SIG39
                UV_SIGCASE( SIG39 )
#endif
#ifdef SIG40
                UV_SIGCASE( SIG40 )
#endif
#ifdef SIG41
                UV_SIGCASE( SIG41 )
#endif
#ifdef SIG42
                UV_SIGCASE( SIG42 )
#endif
#ifdef SIG43
                UV_SIGCASE( SIG43 )
#endif
#ifdef SIG44
                UV_SIGCASE( SIG44 )
#endif
#ifdef SIG45
                UV_SIGCASE( SIG45 )
#endif
#ifdef SIG46
                UV_SIGCASE( SIG46 )
#endif
#ifdef SIG47
                UV_SIGCASE( SIG47 )
#endif
#ifdef SIG48
                UV_SIGCASE( SIG48 )
#endif
#ifdef SIG49
                UV_SIGCASE( SIG49 )
#endif
#ifdef SIG50
                UV_SIGCASE( SIG50 )
#endif
#ifdef SIG51
                UV_SIGCASE( SIG51 )
#endif
#ifdef SIG52
                UV_SIGCASE( SIG52 )
#endif
#ifdef SIG53
                UV_SIGCASE( SIG53 )
#endif
#ifdef SIG54
                UV_SIGCASE( SIG54 )
#endif
#ifdef SIG55
                UV_SIGCASE( SIG55 )
#endif
#ifdef SIG56
                UV_SIGCASE( SIG56 )
#endif
#ifdef SIG57
                UV_SIGCASE( SIG57 )
#endif
#ifdef SIG58
                UV_SIGCASE( SIG58 )
#endif
#ifdef SIG59
                UV_SIGCASE( SIG59 )
#endif
#ifdef SIG60
                UV_SIGCASE( SIG60 )
#endif
#ifdef SIG61
                UV_SIGCASE( SIG61 )
#endif
#ifdef SIG62
                UV_SIGCASE( SIG62 )
#endif
#ifdef SIG63
                UV_SIGCASE( SIG63 )
#endif
#ifdef SIGCANCEL
                UV_SIGCASE( SIGCANCEL )
#endif
#ifdef SIG32
                UV_SIGCASE( SIG32 )
#endif
#ifdef SIG64
                UV_SIGCASE( SIG64 )
#endif
#ifdef SIG65
                UV_SIGCASE( SIG65 )
#endif
#ifdef SIG66
                UV_SIGCASE( SIG66 )
#endif
#ifdef SIG67
                UV_SIGCASE( SIG67 )
#endif
#ifdef SIG68
                UV_SIGCASE( SIG68 )
#endif
#ifdef SIG69
                UV_SIGCASE( SIG69 )
#endif
#ifdef SIG70
                UV_SIGCASE( SIG70 )
#endif
#ifdef SIG71
                UV_SIGCASE( SIG71 )
#endif
#ifdef SIG72
                UV_SIGCASE( SIG72 )
#endif
#ifdef SIG73
                UV_SIGCASE( SIG73 )
#endif
#ifdef SIG74
                UV_SIGCASE( SIG74 )
#endif
#ifdef SIG75
                UV_SIGCASE( SIG75 )
#endif
#ifdef SIG76
                UV_SIGCASE( SIG76 )
#endif
#ifdef SIG77
                UV_SIGCASE( SIG77 )
#endif
#ifdef SIG78
                UV_SIGCASE( SIG78 )
#endif
#ifdef SIG79
                UV_SIGCASE( SIG79 )
#endif
#ifdef SIG80
                UV_SIGCASE( SIG80 )
#endif
#ifdef SIG81
                UV_SIGCASE( SIG81 )
#endif
#ifdef SIG82
                UV_SIGCASE( SIG82 )
#endif
#ifdef SIG83
                UV_SIGCASE( SIG83 )
#endif
#ifdef SIG84
                UV_SIGCASE( SIG84 )
#endif
#ifdef SIG85
                UV_SIGCASE( SIG85 )
#endif
#ifdef SIG86
                UV_SIGCASE( SIG86 )
#endif
#ifdef SIG87
                UV_SIGCASE( SIG87 )
#endif
#ifdef SIG88
                UV_SIGCASE( SIG88 )
#endif
#ifdef SIG89
                UV_SIGCASE( SIG89 )
#endif
#ifdef SIG90
                UV_SIGCASE( SIG90 )
#endif
#ifdef SIG91
                UV_SIGCASE( SIG91 )
#endif
#ifdef SIG92
                UV_SIGCASE( SIG92 )
#endif
#ifdef SIG93
                UV_SIGCASE( SIG93 )
#endif
#ifdef SIG94
                UV_SIGCASE( SIG94 )
#endif
#ifdef SIG95
                UV_SIGCASE( SIG95 )
#endif
#ifdef SIG96
                UV_SIGCASE( SIG96 )
#endif
#ifdef SIG97
                UV_SIGCASE( SIG97 )
#endif
#ifdef SIG98
                UV_SIGCASE( SIG98 )
#endif
#ifdef SIG99
                UV_SIGCASE( SIG99 )
#endif
#ifdef SIG100
                UV_SIGCASE( SIG100 )
#endif
#ifdef SIG101
                UV_SIGCASE( SIG101 )
#endif
#ifdef SIG102
                UV_SIGCASE( SIG102 )
#endif
#ifdef SIG103
                UV_SIGCASE( SIG103 )
#endif
#ifdef SIG104
                UV_SIGCASE( SIG104 )
#endif
#ifdef SIG105
                UV_SIGCASE( SIG105 )
#endif
#ifdef SIG106
                UV_SIGCASE( SIG106 )
#endif
#ifdef SIG107
                UV_SIGCASE( SIG107 )
#endif
#ifdef SIG108
                UV_SIGCASE( SIG108 )
#endif
#ifdef SIG109
                UV_SIGCASE( SIG109 )
#endif
#ifdef SIG110
                UV_SIGCASE( SIG110 )
#endif
#ifdef SIG111
                UV_SIGCASE( SIG111 )
#endif
#ifdef SIG112
                UV_SIGCASE( SIG112 )
#endif
#ifdef SIG113
                UV_SIGCASE( SIG113 )
#endif
#ifdef SIG114
                UV_SIGCASE( SIG114 )
#endif
#ifdef SIG115
                UV_SIGCASE( SIG115 )
#endif
#ifdef SIG116
                UV_SIGCASE( SIG116 )
#endif
#ifdef SIG117
                UV_SIGCASE( SIG117 )
#endif
#ifdef SIG118
                UV_SIGCASE( SIG118 )
#endif
#ifdef SIG119
                UV_SIGCASE( SIG119 )
#endif
#ifdef SIG120
                UV_SIGCASE( SIG120 )
#endif
#ifdef SIG121
                UV_SIGCASE( SIG121 )
#endif
#ifdef SIG122
                UV_SIGCASE( SIG122 )
#endif
#ifdef SIG123
                UV_SIGCASE( SIG123 )
#endif
#ifdef SIG124
                UV_SIGCASE( SIG124 )
#endif
#ifdef SIG125
                UV_SIGCASE( SIG125 )
#endif
#ifdef SIG126
                UV_SIGCASE( SIG126 )
#endif
#ifdef SIG127
                UV_SIGCASE( SIG127 )
#endif
#ifdef EXC_BAD_ACCESS
                UV_SIGCASE( EXC_BAD_ACCESS )
#endif
#ifdef EXC_BAD_INSTRUCTION
                UV_SIGCASE( EXC_BAD_INSTRUCTION )
#endif
#ifdef EXC_ARITHMETIC
                UV_SIGCASE( EXC_ARITHMETIC )
#endif
#ifdef EXC_EMULATION
                UV_SIGCASE( EXC_EMULATION )
#endif
#ifdef EXC_SOFTWARE
                UV_SIGCASE( EXC_SOFTWARE )
#endif
#ifdef EXC_BREAKPOINT
                UV_SIGCASE( EXC_BREAKPOINT )
#endif
#undef UV_SIGCASE
                default:
                    signame = "<UNKNOWN>";
            }

            return std::string( signame );
#else
            std::string signame = strsignal( signum );

            //Ensure it's uppercase
            std::transform( signame.begin(), signame.end(), signame.begin(), ::toupper );

            return signame;
#endif
        }
    }
}

#endif //UV_HANDLE_DETAIL_HPP
