#include <iostream>
#include <cstring>
#include <map>
#include "src/archive.h"
#include "src/log.h"

#define PROGNAME "tomoconv"

#if defined(_MSC_VER) && _MSC_VER
#   include <Windows.h>
#   include <io.h>
#   define unreachable() __assume(0)

#   undef ERROR /* Come on guys */

#else
#   define unreachable() __builtin_unreachable()

#endif

#if __has_cpp_attribute(assume)
/* No way Microsoft implemented this already */
#   pragma message "They actually did it"
#   undef unreachable
#   define unreachable() [[assume(false)]]

#endif

#if defined(__cpp_consteval) && __cpp_consteval
#   define TOMO_CONSTEVAL consteval

#else
#   define TOMO_CONSTEVAL constexpr

#endif

using namespace std::literals;


class args {
    int argc, argi;
    char **argv;

    std::filesystem::path xml;  /* The only unqualified argument. Parsing stops
                                when this is read */

    std::filesystem::path dir;  /* The argument to -o, --out-dir */
    bool no_lookup;             /* -s, --skip-mrn */
    bool testing_only;          /* If -t, --test is passed */
    bool found_path;            /* I could use a std::optional but not gonna */

    const char *host;
    uint16_t port;

    tomo::log::level lthresh;   /* Logging level override */

    enum type {
        SHORT,  /* short option, prefixed with a single '-' */
        LONG,   /* long option, prefixed with two '-' "--" */
        ARG     /* neither of the above */
    };

    /** This takes advantage of the format of C cmdargs on the target system */
    static type argtype(const char *arg) noexcept;

    char *next() noexcept { return argv[++argi]; }

    bool read_output_dir() noexcept;
    bool read_hostname() noexcept;
    bool read_port() noexcept;
    bool read_loglvl() noexcept;

    void read_short();
    void read_long();
    void read_arg();

public:
    args(int argc, char *argv[]);

    void parse();

    const std::filesystem::path &xml_path() const noexcept { return xml; }
    const std::filesystem::path &out_path() const noexcept { return dir; }

    bool testing() const noexcept { return testing_only; }

    /* This method name is confusing, considering the variable it refs */
    bool skip_lookup() const noexcept { return no_lookup; }

    const char *mrn_hostname() const noexcept { return host; }
    uint16_t mrn_port() const noexcept { return port; }

    tomo::log::level loglvl() const noexcept { return lthresh; }
};


args::type args::argtype(const char *arg) noexcept
{
    if (arg[0] == '-') {
        if (arg[1] == '-') {
            if (arg[2]) {
                return LONG;
            }
        } else if (arg[1]) {
            return SHORT;
        }
    }
    return ARG;
}


bool args::read_output_dir() noexcept
{
    const char *arg;

    arg = next();
    if (arg && argtype(arg) == ARG) {
        dir = arg;
        return true;
    }
    return false;
}


bool args::read_hostname() noexcept
{
    const char *arg;

    arg = next();
    if (arg && argtype(arg) == ARG) {
        host = arg;
        return true;
    }
    return false;
}


bool args::read_port() noexcept
{
    const char *arg;

    arg = next();
    if (arg && argtype(arg) == ARG) {
        port = atoi(arg);
        return true;
    }
    return false;
}


bool args::read_loglvl() noexcept
{
    const char *arg;

    arg = next();
    if (arg && argtype(arg) == ARG) {
        lthresh = std::clamp<tomo::log::level>(static_cast<tomo::log::level>(atoi(arg)),
                                               tomo::log::DEBUG,
                                               tomo::log::ERROR);
        return true;
    }
    return false;
}


void args::read_short()
{
    const char *arg = argv[argi] + 1;
    size_t nopt;

    nopt = strlen(arg);
    for (; *arg; arg++) {
        switch (*arg) {
        case 'o':
            if (nopt == 1) {
                if (read_output_dir()) {
                    return;
                }
            }
            throw std::runtime_error("Short option -o requires an argument");
        case 't':
            testing_only = true;
            break;
        case 'h':
            if (nopt == 1) {
                if (read_hostname()) {
                    return;
                }
            }
            throw std::runtime_error("Short option -h requires an argument");
        case 'p':
            if (nopt == 1) {
                if (read_port()) {
                    return;
                }
            }
            throw std::runtime_error("Short option -p requires an argument");
        case 's':
            no_lookup = true;
            break;
        case 'l':
            if (nopt == 1) {
                if (read_loglvl()) {
                    return;
                }
            }
            throw std::runtime_error("Short option -l requires an argument");
        default:
            tomo::log::printf(tomo::log::WARN, "Unrecognized short option %c", *arg);
            break;
        }
    }
}


void args::read_long()
{
    using map_t = std::map<std::string, int>;
    static const map_t map = {
        { "out-dir"s, 0 },
        { "test"s, 1 },
        { "host"s, 2 },
        { "port"s, 3 },
        { "skip-mrn"s, 4 },
        { "log-lvl", 5 }
    };
    const char *arg = argv[argi] + 2;
    map_t::const_iterator it;

    it = map.find(arg);
    if (it != map.end()) {
        switch (it->second) {
        case 0:
            if (!read_output_dir()) {
                throw std::runtime_error("Option --out-dir requires an argument");
            }
            break;
        case 1:
            testing_only = true;
            break;
        case 2:
            if (!read_hostname()) {
                throw std::runtime_error("Option --host requires an argument");
            }
            break;
        case 3:
            if (!read_port()) {
                throw std::runtime_error("Option --port requires an argument");
            }
            break;
        case 4:
            no_lookup = true;
            break;
        case 5:
            if (!read_loglvl()) {
                throw std::runtime_error("Option --log-lvl requires an argument");
            }
            break;
        default:
            unreachable();
            break;
        }
    } else {
        tomo::log::printf(tomo::log::WARN, "Unrecognized long option %s", arg);
    }
}


void args::read_arg()
{
    xml = argv[argi];
    found_path = true;
    argi = argc - 1;
}


args::args(int argc, char *argv[]):
    argc(argc),
    argi(0),
    argv(argv),
    dir("."),
    no_lookup(false),
    testing_only(false),
    found_path(false),
    host("localhost"),
    port(6006),
    lthresh(tomo::log::DEBUG)
{

}


void args::parse()
{
    char *arg;

    while ((arg = next())) {
        switch (argtype(arg)) {
        case SHORT:
            read_short();
            break;
        case LONG:
            read_long();
            break;
        case ARG:
            read_arg();
            break;
        }
    }
    if (!found_path) {
        throw std::runtime_error("Archive XML path is required");
    }
}


class main_log: public tomo::logfile {
    const char *msg;
    int indent;
    FILE *fp;


    /** Match Windows' default I guess, idc really about overrunning my Linux
     *  terminal
     */
    static TOMO_CONSTEVAL int term_width() { return 120; }


    /** @brief Writes the next line to the output file, line breaking on a word
     *      boundary before overrunning the terminal width. If the next line
     *      ends the message, this does not add a line break. If the message
     *      ends, the message pointer will be left pointing at the nul term
     */
    void put_line() noexcept;

public:
    main_log(tomo::log::level lvl) noexcept;

    tomo::log::level &threshold() noexcept { return tomo::logfile::threshold(); }

    virtual int write(tomo::log::level lvl, const char *msg) noexcept override;
};


main_log::main_log(tomo::log::level lvl) noexcept:
    logfile(lvl)
{

}


#if defined(_MSC_VER) && _MSC_VER

static class fix_printf {

public:
    fix_printf() noexcept { _set_printf_count_output(1); }

} fixprintf;


class termcolor {
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hcons;

public:
    termcolor(FILE *fp, tomo::log::level lvl, bool bold) noexcept:
        info({ 0 }),
        hcons(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fp))))
    {
        WORD attr = 0;

        GetConsoleScreenBufferInfo(hcons, &info);
        switch (lvl) {
        case tomo::log::DEBUG:
            attr = FOREGROUND_BLUE;
            break;
        case tomo::log::INFO:
            attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            break;
        case tomo::log::WARN:
            attr = FOREGROUND_RED | FOREGROUND_GREEN;
            break;
        case tomo::log::ERROR:
            attr = FOREGROUND_RED;
            break;
        }
        /* if (bold) { */
            attr |= FOREGROUND_INTENSITY;
        /* } */
        SetConsoleTextAttribute(hcons, attr);
    }

    ~termcolor()
    {
        SetConsoleTextAttribute(hcons, info.wAttributes);
    }
};

#else
class termcolor {
    FILE *file;

public:
    termcolor(FILE *fp, tomo::log::level lvl, bool bold) noexcept:
        file(fp)
    {
        switch (lvl) {
        case tomo::log::DEBUG:
            fputs("\e[94m", fp);
            break;
        case tomo::log::INFO:
            break;
        case tomo::log::WARN:
            fputs("\e[93m", fp);
            break;
        case tomo::log::ERROR:
            fputs("\e[91m", fp);
            break;
        }
        if (bold) {
            fputs("\e[1m", fp);
        }
    }

    ~termcolor()
    {
        fputs("\e[0m", file);
    }
};

#endif


static const char *lstrip(const char *text) noexcept
{
    while (isspace(*text)) {
        text++;
    }
    return text;
}


static const char *word_boundary(const char *text) noexcept
{
    text = lstrip(text);
    while (!isspace(*text) && *text) {
        text++;
    }
    return text;
}


void main_log::put_line() noexcept
{
    const char *end, *next;
    ptrdiff_t len;
    int space;

    space = term_width() - indent;
    next = word_boundary(msg);
    do {
        end = next;
        next = word_boundary(next);
        len = next - msg;
    } while (*end && len <= space);
    len = end - msg;
    fwrite(msg, 1UL, len, fp);
    msg = lstrip(end);
    if (*msg) {
        fprintf(fp, "\n%*s", indent, "");
    }
}


int main_log::write(tomo::log::level lvl, const char *msg) noexcept
{
    const char *prefix = "";
    int n;

    fp = (lvl < tomo::log::WARN) ? stdout : stderr;
    this->msg = msg;
    switch (lvl) {
    case tomo::log::DEBUG:
        prefix = "debug: ";
        break;
    case tomo::log::INFO:
        break;
    case tomo::log::WARN:
        prefix = "warning: ";
        break;
    case tomo::log::ERROR:
        prefix = "error: ";
        break;
    }
    fprintf(fp, PROGNAME ": %n", &indent);
    {
        termcolor colr(fp, lvl, true);

        fprintf(fp, "%s%n", prefix, &n);
        indent += n;
    }
    {
        termcolor colr(fp, lvl, false);

        do {
            put_line();
        } while (*this->msg);
        putchar('\n');
    }
    return 0;
}


static void print_usage()
{
    static const char *usage =
    "Usage: " PROGNAME " [OPTION] FILE\n"
    "Use a TomoTherapy patient plan xml FILE to convert the archive to DICOM format.\n"
    "If not supplied, the DICOM files will be written to the current directory.\n"
    "\n"
    "Options:\n"
    "    -t, --test             do not write files to the output directory\n"
    "    -o, --out-dir DIR      provides an output DIR for the resulting DICOMs\n"
    "    -h, --host HOST        use hostname HOST for MRN lookups (default localhost)\n"
    "    -p, --port PORT        use port PORT for MRN lookups (default 6006)\n"
    "    -s, --skip-mrn         demote MRN lookup errors to warnings and ignore\n"
    "    -l, --log-lvl LVL      override log level threshold to LVL\n";

    puts(usage);
    {
        termcolor colr(stdout, tomo::log::ERROR, true);
        
        fputs("warning: ", stdout);
    }
    {
        termcolor colr(stdout, tomo::log::ERROR, false);

        puts("Exercise due care when supplying a HOST name for the MRN server. The\n"
             "communication protocol employed by this application sends plain-text patient\n"
             "information across transport endpoints. Be sure that you trust the remote server");
    }
}


int main(int argc, char *argv[])
{
    std::filesystem::path ptxml;
    tomo::archive arch;
    args args(argc, argv);
    main_log log(tomo::log::DEBUG);

    tomo::log::add(log);

    try {
        args.parse();
    } catch (std::runtime_error &e) {
        tomo::log::puts(tomo::log::ERROR, e.what());
        print_usage();
        return 1;
    }

    log.threshold() = args.loglvl();

    if (!args.testing() && !std::filesystem::exists(args.out_path())) {
        tomo::log << tomo::log::ERROR << "Directory " << args.out_path() << " does not exist. Create it before continuing.";
        return 1;
    }

    if (args.testing()) {
        /* Hmm... */
        /* log.threshold() = tomo::log::DEBUG; */
        tomo::log::puts(tomo::log::INFO, "Entering testing mode, no files will be written to disk");
    }

    try {
        arch.load_file(args.xml_path());
        try {
            arch.update_mrn(args.mrn_hostname(), args.mrn_port());
        } catch (std::runtime_error &e) {
            if (args.skip_lookup()) {
                /* Issue a warning that the MRN could not be looked up */
                tomo::log::printf(tomo::log::WARN, e.what());

            } else {
                /* Throw to the lower handler */
                throw e;

            }
        }
        arch.flush(args.out_path(), args.testing());
        return 0;

    } catch (const tomo::parse_error &e) {
        tomo::log::printf(tomo::log::ERROR, "Failed to parse XML %s: %s", e.path().string().c_str(), e.res().description());
        //tomo::log << tomo::log::ERROR << "Failed to parse XML " << e.path() << ": " << e.res().description();

    } catch (const tomo::missing_keys &e) {
        tomo::log::printf(tomo::log::ERROR, "Node %s is missing required keys:", e.root().name());
        for (const auto &key: e.keys()) {
            tomo::log::printf(tomo::log::ERROR, "   - %s", key.c_str());
        }

    } catch (std::runtime_error &e) {
        tomo::log::puts(tomo::log::ERROR, e.what());

    }
    return 1;
}
