//
// Created by Aaron on 7/27/2016.
//

#ifndef UV_HANDLE_DETAIL_HPP
#define UV_HANDLE_DETAIL_HPP

//For strsignal if available
#include <csignal>
#include <cstring>

#include <string>

namespace uv {
    namespace detail {
        /*
         * So this is here because MinGW doesn't have a working strsignal implementation for some reason, so on MinGW
         * I have to manually list every single possible signal, even Windows specific ones
         *
         * The current list was taken from https://en.wikipedia.org/wiki/Unix_signal
         *
         * If I'm missing any, feel free to open a pull request or just an issue.
         * */
        std::string signame( int signum ) {
#ifdef __MINGW32__
            const char *signame;
            switch( signum ) {
#define UV_SIGCASE( SIGNAL ) case SIGNAL: signame = #SIGNAL; break;

#ifdef SIGABRT
                UV_SIGCASE( SIGABRT )
#endif
#ifdef SIGALRM
                UV_SIGCASE(SIGALRM)
#endif
#ifdef SIGBUS
                UV_SIGCASE(SIGBUS)
#endif
#ifdef SIGCHLD
                UV_SIGCASE(SIGCHLD)
#endif
#ifdef SIGCONT
                UV_SIGCASE(SIGCONT)
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
                UV_SIGCASE(SIGPIPE)
#endif
#ifdef SIGPOLL
                UV_SIGCASE(SIGPOLL)
#endif
#ifdef SIGPROF
                UV_SIGCASE(SIGPROF)
#endif
#ifdef SIGQUIT
                UV_SIGCASE(SIGQUIT)
#endif
#ifdef SIGSEGV
                UV_SIGCASE( SIGSEGV )
#endif
#ifdef SIGSTOP
                UV_SIGCASE(SIGSTOP)
#endif
#ifdef SIGSYS
                UV_SIGCASE(SIGSYS)
#endif
#ifdef SIGTERM
                UV_SIGCASE( SIGTERM )
#endif
#ifdef SIGTRAP
                UV_SIGCASE(SIGTRAP)
#endif
#ifdef SIGTSTP
                UV_SIGCASE(SIGTSTP)
#endif
#ifdef SIGTTIN
                UV_SIGCASE(SIGTTIN)
#endif
#ifdef SIGTTOU
                UV_SIGCASE(SIGTTOU)
#endif
#ifdef SIGUSR1
                UV_SIGCASE(SIGUSR1)
#endif
#ifdef SIGUSR2
                UV_SIGCASE(SIGUSR2)
#endif
#ifdef SIGURG
                UV_SIGCASE(SIGURG)
#endif
#ifdef SIGVTALRM
                UV_SIGCASE(SIGVTALRM)
#endif
#ifdef SIGXCPU
                UV_SIGCASE(SIGXCPU)
#endif
#ifdef SIGXFSZ
                UV_SIGCASE(SIGXFSZ)
#endif
#ifdef SIGEMT
                UV_SIGCASE(SIGEMT)
#endif
#ifdef SIGINFO
                UV_SIGCASE(SIGINFO)
#endif
#ifdef SIGPWR
                UV_SIGCASE(SIGPWR)
#endif
#ifdef SIGLOST
                UV_SIGCASE(SIGLOST)
#endif
#ifdef SIGWINCH
                UV_SIGCASE( SIGWINCH )
#endif

#undef UV_SIGCASE
                default:
                    signame = "<UNKNOWN>";
            }

            return std::string( signame );
#else
            return strsignal(signum);
#endif
        }
    }
}

#endif //UV_HANDLE_DETAIL_HPP
