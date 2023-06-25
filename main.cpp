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

#   undef ERROR /* Come on guys what the fuck */

#else
#   define unreachable() __builtin_unreachable()

#endif

#if __has_cpp_attribute(assume)
/* If Microsoft implemented this already imma shit my pants */
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
    bool testing_only;          /* If -t, --test is passed */
    bool found_path;            /* I could use a std::optional but sometimes I
                                really don't feel like fielding C++'s pretense */

    enum type {
        SHORT,  /* short option, prefixed with a single '-' */
        LONG,   /* long option, prefixed with two '-' "--" */
        ARG     /* neither of the above */
    };

    /** This takes advantage of the format of C cmdargs on the target system */
    static type argtype(const char *arg);

    char *next() noexcept { return argv[++argi]; }

    bool read_output_dir();

    void read_short();
    void read_long();
    void read_arg();

public:
    args(int argc, char *argv[]);

    void parse();

    const std::filesystem::path &xml_path() const noexcept { return xml; }
    const std::filesystem::path &out_path() const noexcept { return dir; }

    bool testing() const noexcept { return testing_only; }
};


args::type args::argtype(const char *arg)
{
    if (arg[0] == '-') {
        switch (arg[1]) {
        case '-':
            switch (arg[2]) {
            case '\0':
                /* The argument is just "--" */
                return ARG;
            default:
                /* The argument is a string prefixed with "--" */
                return LONG;
            }
        case '\0':
            /* The argument is just '-' */
            return ARG;
        default:
            /* The argument is a string prefixed with '-' */
            return SHORT;
        }
    } else {
        /* Doesn't start with '-' */
        return ARG;
    }
}


bool args::read_output_dir()
{
    const char *arg;

    arg = next();
    if (arg && argtype(arg) == ARG) {
        dir = arg;
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
                    break;
                }
            }
            throw std::runtime_error("Short option -o requires an argument");
        case 't':
            testing_only = true;
            break;
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
        { "test"s, 1 }
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
    testing_only(false),
    found_path(false)
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
/** This shall be exported as a global symbol with a plain-text C-linkage name.
 *  My hubris is profound, but my satisfaction immeasurable
 */
extern "C" void unfuck_printf(void)
{
    _set_printf_count_output(1);
}

static class unfuck_printf {    /* Un-fucking-real */

public:
    unfuck_printf() noexcept { ::unfuck_printf(); }

} unfuck;  /* The names shall remain until my petulant indignation dies down */


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
        if (bold) {
            /* Not really but it's cool that I'm at least using it */
            attr |= FOREGROUND_INTENSITY;
        }
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
    "    -o, --out-dir DIR      provides an output DIR for the resulting DICOMs\n";

    puts(usage);
}


int main(int argc, char *argv[])
{
    std::filesystem::path ptxml;
    tomo::archive arch;
    args args(argc, argv);
    main_log log(tomo::log::WARN);

    tomo::log::add(log);

    try {
        args.parse();
    } catch (std::runtime_error &e) {
        tomo::log::puts(tomo::log::ERROR, e.what());
        print_usage();
        return 1;
    }

    if (!args.testing() && !std::filesystem::exists(args.out_path())) {
        tomo::log << tomo::log::ERROR << "Directory " << args.out_path() << " does not exist. Create it before continuing.";
        return 1;
    }

    if (args.testing()) {
        /* Hmm... */
        log.threshold() = tomo::log::DEBUG;
        tomo::log::puts(tomo::log::INFO, "Entering testing mode, no files shall be written to disk");
    }

    try {
        arch.load_file(args.xml_path());
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

    } catch (std::exception &e) {
        /* I want a stack trace so don't actually do anything */
        throw e;

    }
    return 1;
}
