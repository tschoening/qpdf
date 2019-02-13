#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

#include <qpdf/QUtil.hh>
#include <qpdf/QTC.hh>
#include <qpdf/ClosedFileInputSource.hh>
#include <qpdf/FileInputSource.hh>
#include <qpdf/Pl_StdioFile.hh>
#include <qpdf/Pl_Discard.hh>
#include <qpdf/Pl_DCT.hh>
#include <qpdf/Pl_Count.hh>
#include <qpdf/PointerHolder.hh>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFPageLabelDocumentHelper.hh>
#include <qpdf/QPDFOutlineDocumentHelper.hh>
#include <qpdf/QPDFAcroFormDocumentHelper.hh>
#include <qpdf/QPDFExc.hh>

#include <qpdf/QPDFWriter.hh>

static int const EXIT_ERROR = 2;
static int const EXIT_WARNING = 3;

static char const* whoami = 0;

struct PageSpec
{
    PageSpec(std::string const& filename,
             char const* password,
             char const* range) :
        filename(filename),
        password(password),
        range(range)
    {
    }

    std::string filename;
    char const* password;
    char const* range;
};

struct RotationSpec
{
    RotationSpec(int angle = 0, bool relative = false) :
        angle(angle),
        relative(relative)
    {
    }

    int angle;
    bool relative;
};

enum password_mode_e { pm_bytes, pm_hex_bytes, pm_unicode, pm_auto };

struct UnderOverlay
{
    UnderOverlay(char const* which) :
        which(which),
        filename(0),
        password(0),
        to_nr("1-z"),
        from_nr("1-z"),
        repeat_nr("")
    {
    }

    std::string which;
    char const* filename;
    char const* password;
    char const* to_nr;
    char const* from_nr;
    char const* repeat_nr;
    PointerHolder<QPDF> pdf;
    std::vector<int> to_pagenos;
    std::vector<int> from_pagenos;
    std::vector<int> repeat_pagenos;
};

struct Options
{
    Options() :
        password(0),
        linearize(false),
        decrypt(false),
        split_pages(0),
        verbose(false),
        progress(false),
        suppress_warnings(false),
        copy_encryption(false),
        encryption_file(0),
        encryption_file_password(0),
        encrypt(false),
        password_is_hex_key(false),
        suppress_password_recovery(false),
        password_mode(pm_auto),
        keylen(0),
        r2_print(true),
        r2_modify(true),
        r2_extract(true),
        r2_annotate(true),
        r3_accessibility(true),
        r3_extract(true),
        r3_assemble(true),
        r3_annotate_and_form(true),
        r3_form_filling(true),
        r3_modify_other(true),
        r3_print(qpdf_r3p_full),
        force_V4(false),
        force_R5(false),
        cleartext_metadata(false),
        use_aes(false),
        stream_data_set(false),
        stream_data_mode(qpdf_s_compress),
        compress_streams(true),
        compress_streams_set(false),
        decode_level(qpdf_dl_generalized),
        decode_level_set(false),
        normalize_set(false),
        normalize(false),
        suppress_recovery(false),
        object_stream_set(false),
        object_stream_mode(qpdf_o_preserve),
        ignore_xref_streams(false),
        qdf_mode(false),
        preserve_unreferenced_objects(false),
        preserve_unreferenced_page_resources(false),
        keep_files_open(true),
        keep_files_open_set(false),
        newline_before_endstream(false),
        coalesce_contents(false),
        flatten_annotations(false),
        flatten_annotations_required(0),
        flatten_annotations_forbidden(an_invisible | an_hidden),
        generate_appearances(false),
        show_npages(false),
        deterministic_id(false),
        static_id(false),
        static_aes_iv(false),
        suppress_original_object_id(false),
        show_encryption(false),
        show_encryption_key(false),
        check_linearization(false),
        show_linearization(false),
        show_xref(false),
        show_trailer(false),
        show_obj(0),
        show_gen(0),
        show_raw_stream_data(false),
        show_filtered_stream_data(false),
        show_pages(false),
        show_page_images(false),
        collate(false),
        json(false),
        check(false),
        optimize_images(false),
        externalize_inline_images(false),
        keep_inline_images(false),
        oi_min_width(128),      // Default values for these
        oi_min_height(128),     // oi flags are in --help
        oi_min_area(16384),     // and in the manual.
        ii_min_bytes(1024),     //
        underlay("underlay"),
        overlay("overlay"),
        under_overlay(0),
        require_outfile(true),
        infilename(0),
        outfilename(0)
    {
    }

    char const* password;
    bool linearize;
    bool decrypt;
    int split_pages;
    bool verbose;
    bool progress;
    bool suppress_warnings;
    bool copy_encryption;
    char const* encryption_file;
    char const* encryption_file_password;
    bool encrypt;
    bool password_is_hex_key;
    bool suppress_password_recovery;
    password_mode_e password_mode;
    std::string user_password;
    std::string owner_password;
    int keylen;
    bool r2_print;
    bool r2_modify;
    bool r2_extract;
    bool r2_annotate;
    bool r3_accessibility;
    bool r3_extract;
    bool r3_assemble;
    bool r3_annotate_and_form;
    bool r3_form_filling;
    bool r3_modify_other;
    qpdf_r3_print_e r3_print;
    bool force_V4;
    bool force_R5;
    bool cleartext_metadata;
    bool use_aes;
    bool stream_data_set;
    qpdf_stream_data_e stream_data_mode;
    bool compress_streams;
    bool compress_streams_set;
    qpdf_stream_decode_level_e decode_level;
    bool decode_level_set;
    bool normalize_set;
    bool normalize;
    bool suppress_recovery;
    bool object_stream_set;
    qpdf_object_stream_e object_stream_mode;
    bool ignore_xref_streams;
    bool qdf_mode;
    bool preserve_unreferenced_objects;
    bool preserve_unreferenced_page_resources;
    bool keep_files_open;
    bool keep_files_open_set;
    bool newline_before_endstream;
    std::string linearize_pass1;
    bool coalesce_contents;
    bool flatten_annotations;
    int flatten_annotations_required;
    int flatten_annotations_forbidden;
    bool generate_appearances;
    std::string min_version;
    std::string force_version;
    bool show_npages;
    bool deterministic_id;
    bool static_id;
    bool static_aes_iv;
    bool suppress_original_object_id;
    bool show_encryption;
    bool show_encryption_key;
    bool check_linearization;
    bool show_linearization;
    bool show_xref;
    bool show_trailer;
    int show_obj;
    int show_gen;
    bool show_raw_stream_data;
    bool show_filtered_stream_data;
    bool show_pages;
    bool show_page_images;
    bool collate;
    bool json;
    std::set<std::string> json_keys;
    std::set<std::string> json_objects;
    bool check;
    bool optimize_images;
    bool externalize_inline_images;
    bool keep_inline_images;
    size_t oi_min_width;
    size_t oi_min_height;
    size_t oi_min_area;
    size_t ii_min_bytes;
    UnderOverlay underlay;
    UnderOverlay overlay;
    UnderOverlay* under_overlay;
    std::vector<PageSpec> page_specs;
    std::map<std::string, RotationSpec> rotations;
    bool require_outfile;
    char const* infilename;
    char const* outfilename;
};

struct QPDFPageData
{
    QPDFPageData(std::string const& filename, QPDF* qpdf, char const* range);
    QPDFPageData(QPDFPageData const& other, int page);

    std::string filename;
    QPDF* qpdf;
    std::vector<QPDFObjectHandle> orig_pages;
    std::vector<int> selected_pages;
};

class DiscardContents: public QPDFObjectHandle::ParserCallbacks
{
  public:
    virtual ~DiscardContents() {}
    virtual void handleObject(QPDFObjectHandle) {}
    virtual void handleEOF() {}
};

class ProgressReporter: public QPDFWriter::ProgressReporter
{
  public:
    ProgressReporter(char const* filename) :
        filename(filename)
    {
    }
    virtual ~ProgressReporter()
    {
    }

    virtual void reportProgress(int);
  private:
    std::string filename;
};

void
ProgressReporter::reportProgress(int percentage)
{
    std::cout << whoami << ": " << filename << ": write progress: "
              << percentage << "%" << std::endl;
}

static JSON json_schema(std::set<std::string>* keys = 0)
{
    // Style: use all lower-case keys with no dashes or underscores.
    // Choose array or dictionary based on indexing. For example, we
    // use a dictionary for objects because we want to index by object
    // ID and an array for pages because we want to index by position.
    // The pages in the pages array contain references back to the
    // original object, which can be resolved in the objects
    // dictionary. When a PDF construct that maps back to an original
    // object is represented separately, use "object" as the key that
    // references the original object.

    // This JSON object doubles as a schema and as documentation for
    // our JSON output. Any schema mismatch is a bug in qpdf. This
    // helps to enforce our policy of consistently providing a known
    // structure where every documented key will always be present,
    // which makes it easier to consume our JSON. This is discussed in
    // more depth in the manual.
    JSON schema = JSON::makeDictionary();
    schema.addDictionaryMember(
        "version", JSON::makeString(
            "JSON format serial number; increased for non-compatible changes"));
    JSON j_params = schema.addDictionaryMember(
        "parameters", JSON::makeDictionary());
    j_params.addDictionaryMember(
        "decodelevel", JSON::makeString(
            "decode level used to determine stream filterability"));

    bool all_keys = ((keys == 0) || keys->empty());

    // The list of selectable top-level keys id duplicated in three
    // places: json_schema, do_json, and initOptionTable.
    if (all_keys || keys->count("objects"))
    {
        schema.addDictionaryMember(
            "objects", JSON::makeString(
                "dictionary of original objects;"
                " keys are 'trailer' or 'n n R'"));
    }
    if (all_keys || keys->count("pages"))
    {
        JSON page = schema.addDictionaryMember("pages", JSON::makeArray()).
            addArrayElement(JSON::makeDictionary());
        page.addDictionaryMember(
            "object",
            JSON::makeString("reference to original page object"));
        JSON image = page.addDictionaryMember("images", JSON::makeArray()).
            addArrayElement(JSON::makeDictionary());
        image.addDictionaryMember(
            "name",
            JSON::makeString("name of image in XObject table"));
        image.addDictionaryMember(
            "object",
            JSON::makeString("reference to image stream"));
        image.addDictionaryMember(
            "width",
            JSON::makeString("image width"));
        image.addDictionaryMember(
            "height",
            JSON::makeString("image height"));
        image.addDictionaryMember(
            "colorspace",
            JSON::makeString("color space"));
        image.addDictionaryMember(
            "bitspercomponent",
            JSON::makeString("bits per component"));
        image.addDictionaryMember("filter", JSON::makeArray()).
            addArrayElement(
                JSON::makeString("filters applied to image data"));
        image.addDictionaryMember("decodeparms", JSON::makeArray()).
            addArrayElement(
                JSON::makeString("decode parameters for image data"));
        image.addDictionaryMember(
            "filterable",
            JSON::makeString("whether image data can be decoded"
                             " using the decode level qpdf was invoked with"));
        page.addDictionaryMember("contents", JSON::makeArray()).
            addArrayElement(
                JSON::makeString("reference to each content stream"));
        page.addDictionaryMember(
            "label",
            JSON::makeString("page label dictionary, or null if none"));
        JSON outline = page.addDictionaryMember("outlines", JSON::makeArray()).
            addArrayElement(JSON::makeDictionary());
        outline.addDictionaryMember(
            "object",
            JSON::makeString("reference to outline that targets this page"));
        outline.addDictionaryMember(
            "title",
            JSON::makeString("outline title"));
        outline.addDictionaryMember(
            "dest",
            JSON::makeString("outline destination dictionary"));
        page.addDictionaryMember(
            "pageposfrom1",
            JSON::makeString("position of page in document numbering from 1"));
    }
    if (all_keys || keys->count("pagelabels"))
    {
        JSON labels = schema.addDictionaryMember(
            "pagelabels", JSON::makeArray()).
            addArrayElement(JSON::makeDictionary());
        labels.addDictionaryMember(
            "index",
            JSON::makeString("starting page position starting from zero"));
        labels.addDictionaryMember(
            "label",
            JSON::makeString("page label dictionary"));
    }
    if (all_keys || keys->count("outlines"))
    {
        JSON outlines = schema.addDictionaryMember(
            "outlines", JSON::makeArray()).
            addArrayElement(JSON::makeDictionary());
        outlines.addDictionaryMember(
            "object",
            JSON::makeString("reference to this outline"));
        outlines.addDictionaryMember(
            "title",
            JSON::makeString("outline title"));
        outlines.addDictionaryMember(
            "dest",
            JSON::makeString("outline destination dictionary"));
        outlines.addDictionaryMember(
            "kids",
            JSON::makeString("array of descendent outlines"));
        outlines.addDictionaryMember(
            "open",
            JSON::makeString("whether the outline is displayed expanded"));
        outlines.addDictionaryMember(
            "destpageposfrom1",
            JSON::makeString("position of destination page in document"
                             " numbered from 1; null if not known"));
    }
    if (all_keys || keys->count("acroform"))
    {
        JSON acroform = schema.addDictionaryMember(
            "acroform", JSON::makeDictionary());
        acroform.addDictionaryMember(
            "hasacroform",
            JSON::makeString("whether the document has interactive forms"));
        acroform.addDictionaryMember(
            "needappearances",
            JSON::makeString("whether the form fields' appearance"
                             " streams need to be regenerated"));
        JSON fields = acroform.addDictionaryMember(
            "fields", JSON::makeArray()).
            addArrayElement(JSON::makeDictionary());
        fields.addDictionaryMember(
            "object",
            JSON::makeString("reference to this form field"));
        fields.addDictionaryMember(
            "parent",
            JSON::makeString("reference to this field's parent"));
        fields.addDictionaryMember(
            "pageposfrom1",
            JSON::makeString("position of containing page numbered from 1"));
        fields.addDictionaryMember(
            "fieldtype",
            JSON::makeString("field type"));
        fields.addDictionaryMember(
            "fieldflags",
            JSON::makeString(
                "form field flags from /Ff --"
                " see pdf_form_field_flag_e in qpdf/Constants.h"));
        fields.addDictionaryMember(
            "fullname",
            JSON::makeString("full name of field"));
        fields.addDictionaryMember(
            "partialname",
            JSON::makeString("partial name of field"));
        fields.addDictionaryMember(
            "alternativename",
            JSON::makeString(
                "alternative name of field --"
                " this is the one usually shown to users"));
        fields.addDictionaryMember(
            "mappingname",
            JSON::makeString("mapping name of field"));
        fields.addDictionaryMember(
            "value",
            JSON::makeString("value of field"));
        fields.addDictionaryMember(
            "defaultvalue",
            JSON::makeString("default value of field"));
        fields.addDictionaryMember(
            "quadding",
            JSON::makeString(
                "field quadding --"
                " number indicating left, center, or right"));
        fields.addDictionaryMember(
            "ischeckbox",
            JSON::makeString("whether field is a checkbox"));
        fields.addDictionaryMember(
            "isradiobutton",
            JSON::makeString("whether field is a radio button --"
                             " buttons in a single group share a parent"));
        fields.addDictionaryMember(
            "ischoice",
            JSON::makeString("whether field is a list, combo, or dropdown"));
        fields.addDictionaryMember(
            "istext",
            JSON::makeString("whether field is a text field"));
        JSON j_choices = fields.addDictionaryMember(
            "choices",
            JSON::makeString("for choices fields, the list of"
                             " choices presented to the user"));
        JSON annotation = fields.addDictionaryMember(
            "annotation", JSON::makeDictionary());
        annotation.addDictionaryMember(
            "object",
            JSON::makeString("reference to the annotation object"));
        annotation.addDictionaryMember(
            "appearancestate",
            JSON::makeString("appearance state --"
                             " can be used to determine value for"
                             " checkboxes and radio buttons"));
        annotation.addDictionaryMember(
            "annotationflags",
            JSON::makeString(
                "annotation flags from /F --"
                " see pdf_annotation_flag_e in qpdf/Constants.h"));
    }
    return schema;
}

static void parse_object_id(std::string const& objspec,
                            bool& trailer, int& obj, int& gen)
{
    if (objspec == "trailer")
    {
        trailer = true;
    }
    else
    {
        trailer = false;
        obj = QUtil::string_to_int(objspec.c_str());
        size_t comma = objspec.find(',');
        if ((comma != std::string::npos) && (comma + 1 < objspec.length()))
        {
            gen = QUtil::string_to_int(
                objspec.substr(1 + comma, std::string::npos).c_str());
        }
    }
}

// This is not a general-purpose argument parser. It is tightly
// crafted to work with qpdf. qpdf's command-line syntax is very
// complex because of its long history, and it doesn't really follow
// any kind of normal standard for arguments, but I don't want to
// break compatibility by changing what constitutes a valid command.
// This class is intended to simplify the argument parsing code and
// also to make it possible to add bash completion support while
// guaranteeing consistency with the actual argument syntax.
class ArgParser
{
  public:
    ArgParser(int argc, char* argv[], Options& o);
    void parseOptions();

  private:
    typedef void (ArgParser::*bare_arg_handler_t)();
    typedef void (ArgParser::*param_arg_handler_t)(char* parameter);

    struct OptionEntry
    {
        OptionEntry() :
            parameter_needed(false),
            bare_arg_handler(0),
            param_arg_handler(0)
        {
        }
        bool parameter_needed;
        std::string parameter_name;
        std::set<std::string> choices;
        bare_arg_handler_t bare_arg_handler;
        param_arg_handler_t param_arg_handler;
    };
    friend struct OptionEntry;

    OptionEntry oe_positional(param_arg_handler_t);
    OptionEntry oe_bare(bare_arg_handler_t);
    OptionEntry oe_requiredParameter(param_arg_handler_t, char const* name);
    OptionEntry oe_optionalParameter(param_arg_handler_t);
    OptionEntry oe_requiredChoices(param_arg_handler_t, char const** choices);

    void argHelp();
    void argVersion();
    void argCopyright();
    void argCompletionBash();
    void argCompletionZsh();
    void argJsonHelp();
    void argPositional(char* arg);
    void argPassword(char* parameter);
    void argEmpty();
    void argLinearize();
    void argEncrypt();
    void argDecrypt();
    void argPasswordIsHexKey();
    void argPasswordMode(char* parameter);
    void argSuppressPasswordRecovery();
    void argCopyEncryption(char* parameter);
    void argEncryptionFilePassword(char* parameter);
    void argPages();
    void argUnderlay();
    void argOverlay();
    void argRotate(char* parameter);
    void argCollate();
    void argStreamData(char* parameter);
    void argCompressStreams(char* parameter);
    void argDecodeLevel(char* parameter);
    void argNormalizeContent(char* parameter);
    void argSuppressRecovery();
    void argObjectStreams(char* parameter);
    void argIgnoreXrefStreams();
    void argQdf();
    void argPreserveUnreferenced();
    void argPreserveUnreferencedResources();
    void argKeepFilesOpen(char* parameter);
    void argNewlineBeforeEndstream();
    void argLinearizePass1(char* parameter);
    void argCoalesceContents();
    void argFlattenAnnotations(char* parameter);
    void argGenerateAppearances();
    void argMinVersion(char* parameter);
    void argForceVersion(char* parameter);
    void argSplitPages(char* parameter);
    void argVerbose();
    void argProgress();
    void argNoWarn();
    void argDeterministicId();
    void argStaticId();
    void argStaticAesIv();
    void argNoOriginalObjectIds();
    void argShowEncryption();
    void argShowEncryptionKey();
    void argCheckLinearization();
    void argShowLinearization();
    void argShowXref();
    void argShowObject(char* parameter);
    void argRawStreamData();
    void argFilteredStreamData();
    void argShowNpages();
    void argShowPages();
    void argWithImages();
    void argJson();
    void argJsonKey(char* parameter);
    void argJsonObject(char* parameter);
    void argCheck();
    void argOptimizeImages();
    void argExternalizeInlineImages();
    void argKeepInlineImages();
    void argOiMinWidth(char* parameter);
    void argOiMinHeight(char* parameter);
    void argOiMinArea(char* parameter);
    void argIiMinBytes(char* parameter);
    void arg40Print(char* parameter);
    void arg40Modify(char* parameter);
    void arg40Extract(char* parameter);
    void arg40Annotate(char* parameter);
    void arg128Accessibility(char* parameter);
    void arg128Extract(char* parameter);
    void arg128Print(char* parameter);
    void arg128Modify(char* parameter);
    void arg128ClearTextMetadata();
    void arg128Assemble(char* parameter);
    void arg128Annotate(char* parameter);
    void arg128Form(char* parameter);
    void arg128ModOther(char* parameter);
    void arg128UseAes(char* parameter);
    void arg128ForceV4();
    void arg256ForceR5();
    void argEndEncrypt();
    void argUOpositional(char* arg);
    void argUOto(char* parameter);
    void argUOfrom(char* parameter);
    void argUOrepeat(char* parameter);
    void argUOpassword(char* parameter);
    void argEndUnderOverlay();

    void usage(std::string const& message);
    void checkCompletion();
    void initOptionTable();
    void handleHelpArgs();
    void handleArgFileArguments();
    void handleBashArguments();
    void readArgsFromFile(char const* filename);
    void doFinalChecks();
    void addOptionsToCompletions();
    void addChoicesToCompletions(std::string const&, std::string const&);
    void handleCompletion();
    std::vector<PageSpec> parsePagesOptions();
    void parseUnderOverlayOptions(UnderOverlay*);
    void parseRotationParameter(std::string const&);
    std::vector<int> parseNumrange(char const* range, int max,
                                   bool throw_error = false);

    int argc;
    char** argv;
    Options& o;
    int cur_arg;
    bool bash_completion;
    bool zsh_completion;
    std::string bash_prev;
    std::string bash_cur;
    std::string bash_line;
    std::set<std::string> completions;

    std::map<std::string, OptionEntry>* option_table;
    std::map<std::string, OptionEntry> help_option_table;
    std::map<std::string, OptionEntry> main_option_table;
    std::map<std::string, OptionEntry> encrypt40_option_table;
    std::map<std::string, OptionEntry> encrypt128_option_table;
    std::map<std::string, OptionEntry> encrypt256_option_table;
    std::map<std::string, OptionEntry> under_overlay_option_table;
    std::vector<PointerHolder<char> > new_argv;
    std::vector<PointerHolder<char> > bash_argv;
    PointerHolder<char*> argv_ph;
    PointerHolder<char*> bash_argv_ph;
};

ArgParser::ArgParser(int argc, char* argv[], Options& o) :
    argc(argc),
    argv(argv),
    o(o),
    cur_arg(0),
    bash_completion(false),
    zsh_completion(false)
{
    option_table = &main_option_table;
    initOptionTable();
}

ArgParser::OptionEntry
ArgParser::oe_positional(param_arg_handler_t h)
{
    OptionEntry oe;
    oe.param_arg_handler = h;
    return oe;
}

ArgParser::OptionEntry
ArgParser::oe_bare(bare_arg_handler_t h)
{
    OptionEntry oe;
    oe.parameter_needed = false;
    oe.bare_arg_handler = h;
    return oe;
}

ArgParser::OptionEntry
ArgParser::oe_requiredParameter(param_arg_handler_t h, char const* name)
{
    OptionEntry oe;
    oe.parameter_needed = true;
    oe.parameter_name = name;
    oe.param_arg_handler = h;
    return oe;
}

ArgParser::OptionEntry
ArgParser::oe_optionalParameter(param_arg_handler_t h)
{
    OptionEntry oe;
    oe.parameter_needed = false;
    oe.param_arg_handler = h;
    return oe;
}

ArgParser::OptionEntry
ArgParser::oe_requiredChoices(param_arg_handler_t h, char const** choices)
{
    OptionEntry oe;
    oe.parameter_needed = true;
    oe.param_arg_handler = h;
    for (char const** i = choices; *i; ++i)
    {
        oe.choices.insert(*i);
    }
    return oe;
}

void
ArgParser::initOptionTable()
{
    std::map<std::string, OptionEntry>* t = &this->help_option_table;
    (*t)["help"] = oe_bare(&ArgParser::argHelp);
    (*t)["version"] = oe_bare(&ArgParser::argVersion);
    (*t)["copyright"] = oe_bare(&ArgParser::argCopyright);
    (*t)["completion-bash"] = oe_bare(&ArgParser::argCompletionBash);
    (*t)["completion-zsh"] = oe_bare(&ArgParser::argCompletionZsh);
    (*t)["json-help"] = oe_bare(&ArgParser::argJsonHelp);

    t = &this->main_option_table;
    char const* yn[] = {"y", "n", 0};
    (*t)[""] = oe_positional(&ArgParser::argPositional);
    (*t)["password"] = oe_requiredParameter(
        &ArgParser::argPassword, "password");
    (*t)["empty"] = oe_bare(&ArgParser::argEmpty);
    (*t)["linearize"] = oe_bare(&ArgParser::argLinearize);
    (*t)["encrypt"] = oe_bare(&ArgParser::argEncrypt);
    (*t)["decrypt"] = oe_bare(&ArgParser::argDecrypt);
    (*t)["password-is-hex-key"] = oe_bare(&ArgParser::argPasswordIsHexKey);
    (*t)["suppress-password-recovery"] =
        oe_bare(&ArgParser::argSuppressPasswordRecovery);
    char const* password_mode_choices[] =
        {"bytes", "hex-bytes", "unicode", "auto", 0};
    (*t)["password-mode"] = oe_requiredChoices(
        &ArgParser::argPasswordMode, password_mode_choices);
    (*t)["copy-encryption"] = oe_requiredParameter(
        &ArgParser::argCopyEncryption, "file");
    (*t)["encryption-file-password"] = oe_requiredParameter(
        &ArgParser::argEncryptionFilePassword, "password");
    (*t)["pages"] = oe_bare(&ArgParser::argPages);
    (*t)["rotate"] = oe_requiredParameter(
        &ArgParser::argRotate, "[+|-]angle:page-range");
    char const* stream_data_choices[] =
        {"compress", "preserve", "uncompress", 0};
    (*t)["collate"] = oe_bare(&ArgParser::argCollate);
    (*t)["stream-data"] = oe_requiredChoices(
        &ArgParser::argStreamData, stream_data_choices);
    (*t)["compress-streams"] = oe_requiredChoices(
        &ArgParser::argCompressStreams, yn);
    char const* decode_level_choices[] =
        {"none", "generalized", "specialized", "all", 0};
    (*t)["decode-level"] = oe_requiredChoices(
        &ArgParser::argDecodeLevel, decode_level_choices);
    (*t)["normalize-content"] = oe_requiredChoices(
        &ArgParser::argNormalizeContent, yn);
    (*t)["suppress-recovery"] = oe_bare(&ArgParser::argSuppressRecovery);
    char const* object_streams_choices[] = {
        "disable", "preserve", "generate", 0};
    (*t)["object-streams"] = oe_requiredChoices(
        &ArgParser::argObjectStreams, object_streams_choices);
    (*t)["ignore-xref-streams"] = oe_bare(&ArgParser::argIgnoreXrefStreams);
    (*t)["qdf"] = oe_bare(&ArgParser::argQdf);
    (*t)["preserve-unreferenced"] = oe_bare(
        &ArgParser::argPreserveUnreferenced);
    (*t)["preserve-unreferenced-resources"] = oe_bare(
        &ArgParser::argPreserveUnreferencedResources);
    (*t)["keep-files-open"] = oe_requiredChoices(
        &ArgParser::argKeepFilesOpen, yn);
    (*t)["newline-before-endstream"] = oe_bare(
        &ArgParser::argNewlineBeforeEndstream);
    (*t)["linearize-pass1"] = oe_requiredParameter(
        &ArgParser::argLinearizePass1, "filename");
    (*t)["coalesce-contents"] = oe_bare(&ArgParser::argCoalesceContents);
    char const* flatten_choices[] = {"all", "print", "screen", 0};
    (*t)["flatten-annotations"] = oe_requiredChoices(
        &ArgParser::argFlattenAnnotations, flatten_choices);
    (*t)["generate-appearances"] =
        oe_bare(&ArgParser::argGenerateAppearances);
    (*t)["min-version"] = oe_requiredParameter(
        &ArgParser::argMinVersion, "version");
    (*t)["force-version"] = oe_requiredParameter(
        &ArgParser::argForceVersion, "version");
    (*t)["split-pages"] = oe_optionalParameter(&ArgParser::argSplitPages);
    (*t)["verbose"] = oe_bare(&ArgParser::argVerbose);
    (*t)["progress"] = oe_bare(&ArgParser::argProgress);
    (*t)["no-warn"] = oe_bare(&ArgParser::argNoWarn);
    (*t)["deterministic-id"] = oe_bare(&ArgParser::argDeterministicId);
    (*t)["static-id"] = oe_bare(&ArgParser::argStaticId);
    (*t)["static-aes-iv"] = oe_bare(&ArgParser::argStaticAesIv);
    (*t)["no-original-object-ids"] = oe_bare(
        &ArgParser::argNoOriginalObjectIds);
    (*t)["show-encryption"] = oe_bare(&ArgParser::argShowEncryption);
    (*t)["show-encryption-key"] = oe_bare(&ArgParser::argShowEncryptionKey);
    (*t)["check-linearization"] = oe_bare(&ArgParser::argCheckLinearization);
    (*t)["show-linearization"] = oe_bare(&ArgParser::argShowLinearization);
    (*t)["show-xref"] = oe_bare(&ArgParser::argShowXref);
    (*t)["show-object"] = oe_requiredParameter(
        &ArgParser::argShowObject, "trailer|obj[,gen]");
    (*t)["raw-stream-data"] = oe_bare(&ArgParser::argRawStreamData);
    (*t)["filtered-stream-data"] = oe_bare(&ArgParser::argFilteredStreamData);
    (*t)["show-npages"] = oe_bare(&ArgParser::argShowNpages);
    (*t)["show-pages"] = oe_bare(&ArgParser::argShowPages);
    (*t)["with-images"] = oe_bare(&ArgParser::argWithImages);
    (*t)["json"] = oe_bare(&ArgParser::argJson);
    // The list of selectable top-level keys id duplicated in three
    // places: json_schema, do_json, and initOptionTable.
    char const* json_key_choices[] = {
        "objects", "pages", "pagelabels", "outlines", "acroform", 0};
    (*t)["json-key"] = oe_requiredChoices(
        &ArgParser::argJsonKey, json_key_choices);
    (*t)["json-object"] = oe_requiredParameter(
        &ArgParser::argJsonObject, "trailer|obj[,gen]");
    (*t)["check"] = oe_bare(&ArgParser::argCheck);
    (*t)["optimize-images"] = oe_bare(&ArgParser::argOptimizeImages);
    (*t)["externalize-inline-images"] =
        oe_bare(&ArgParser::argExternalizeInlineImages);
    (*t)["keep-inline-images"] = oe_bare(&ArgParser::argKeepInlineImages);
    (*t)["oi-min-width"] = oe_requiredParameter(
        &ArgParser::argOiMinWidth, "minimum-width");
    (*t)["oi-min-height"] = oe_requiredParameter(
        &ArgParser::argOiMinHeight, "minimum-height");
    (*t)["oi-min-area"] = oe_requiredParameter(
        &ArgParser::argOiMinArea, "minimum-area");
    (*t)["ii-min-bytes"] = oe_requiredParameter(
        &ArgParser::argIiMinBytes, "minimum-bytes");
    (*t)["overlay"] = oe_bare(&ArgParser::argOverlay);
    (*t)["underlay"] = oe_bare(&ArgParser::argUnderlay);

    t = &this->encrypt40_option_table;
    (*t)["--"] = oe_bare(&ArgParser::argEndEncrypt);
    (*t)["print"] = oe_requiredChoices(&ArgParser::arg40Print, yn);
    (*t)["modify"] = oe_requiredChoices(&ArgParser::arg40Modify, yn);
    (*t)["extract"] = oe_requiredChoices(&ArgParser::arg40Extract, yn);
    (*t)["annotate"] = oe_requiredChoices(&ArgParser::arg40Annotate, yn);

    t = &this->encrypt128_option_table;
    (*t)["--"] = oe_bare(&ArgParser::argEndEncrypt);
    (*t)["accessibility"] = oe_requiredChoices(
        &ArgParser::arg128Accessibility, yn);
    (*t)["extract"] = oe_requiredChoices(&ArgParser::arg128Extract, yn);
    char const* print128_choices[] = {"full", "low", "none", 0};
    (*t)["print"] = oe_requiredChoices(
        &ArgParser::arg128Print, print128_choices);
    (*t)["assemble"] = oe_requiredChoices(&ArgParser::arg128Assemble, yn);
    (*t)["annotate"] = oe_requiredChoices(&ArgParser::arg128Annotate, yn);
    (*t)["form"] = oe_requiredChoices(&ArgParser::arg128Form, yn);
    (*t)["modify-other"] = oe_requiredChoices(&ArgParser::arg128ModOther, yn);
    char const* modify128_choices[] =
        {"all", "annotate", "form", "assembly", "none", 0};
    (*t)["modify"] = oe_requiredChoices(
        &ArgParser::arg128Modify, modify128_choices);
    (*t)["cleartext-metadata"] = oe_bare(&ArgParser::arg128ClearTextMetadata);

    // The above 128-bit options are also 256-bit options, so copy
    // what we have so far. Then continue separately with 128 and 256.
    this->encrypt256_option_table = this->encrypt128_option_table;
    (*t)["use-aes"] = oe_requiredChoices(&ArgParser::arg128UseAes, yn);
    (*t)["force-V4"] = oe_bare(&ArgParser::arg128ForceV4);

    t = &this->encrypt256_option_table;
    (*t)["force-R5"] = oe_bare(&ArgParser::arg256ForceR5);

    t = &this->under_overlay_option_table;
    (*t)[""] = oe_positional(&ArgParser::argUOpositional);
    (*t)["to"] = oe_requiredParameter(
        &ArgParser::argUOto, "page-range");
    (*t)["from"] = oe_requiredParameter(
        &ArgParser::argUOfrom, "page-range");
    (*t)["repeat"] = oe_requiredParameter(
        &ArgParser::argUOrepeat, "page-range");
    (*t)["password"] = oe_requiredParameter(
        &ArgParser::argUOpassword, "password");
    (*t)["--"] = oe_bare(&ArgParser::argEndUnderOverlay);
}

void
ArgParser::argPositional(char* arg)
{
    if (o.infilename == 0)
    {
        o.infilename = arg;
    }
    else if (o.outfilename == 0)
    {
        o.outfilename = arg;
    }
    else
    {
        usage(std::string("unknown argument ") + arg);
    }
}

void
ArgParser::argVersion()
{
    std::cout
        << whoami << " version " << QPDF::QPDFVersion() << std::endl
        << "Run " << whoami << " --copyright to see copyright and license information."
        << std::endl;
}

void
ArgParser::argCopyright()
{
    // Make sure the output looks right on an 80-column display.
    //               1         2         3         4         5         6         7         8
    //      12345678901234567890123456789012345678901234567890123456789012345678901234567890
    std::cout
        << whoami << " version " << QPDF::QPDFVersion() << std::endl
        << std::endl
        << "Copyright (c) 2005-2019 Jay Berkenbilt"
        << std::endl
        << "QPDF is licensed under the Apache License, Version 2.0 (the \"License\");"
        << std::endl
        << "not use this file except in compliance with the License."
        << std::endl
        << "You may obtain a copy of the License at"
        << std::endl
        << std::endl
        << "  http://www.apache.org/licenses/LICENSE-2.0"
        << std::endl
        << std::endl
        << "Unless required by applicable law or agreed to in writing, software"
        << std::endl
        << "distributed under the License is distributed on an \"AS IS\" BASIS,"
        << std::endl
        << "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied."
        << std::endl
        << "See the License for the specific language governing permissions and"
        << std::endl
        << "limitations under the License."
        << std::endl
        << std::endl
        << "Versions of qpdf prior to version 7 were released under the terms"
        << std::endl
        << "of version 2.0 of the Artistic License. At your option, you may"
        << std::endl
        << "continue to consider qpdf to be licensed under those terms. Please"
        << std::endl
        << "see the manual for additional information."
        << std::endl;
}

void
ArgParser::argHelp()
{
    std::cout
        // 12345678901234567890123456789012345678901234567890123456789012345678901234567890
        << "Usage: qpdf [ options ] { infilename | --empty } [ outfilename ]\n"
        << "\n"
        << "An option summary appears below.  Please see the documentation for details.\n"
        << "\n"
        << "If @filename appears anywhere in the command-line, each line of filename\n"
        << "will be interpreted as an argument. No interpolation is done. Line\n"
        << "terminators are stripped. @- can be specified to read from standard input.\n"
        << "\n"
        << "Note that when contradictory options are provided, whichever options are\n"
        << "provided last take precedence.\n"
        << "\n"
        << "\n"
        << "Basic Options\n"
        << "-------------\n"
        << "\n"
        << "--version               show version of qpdf\n"
        << "--copyright             show qpdf's copyright and license information\n"
        << "--help                  show command-line argument help\n"
        << "--completion-bash       output a bash complete command you can eval\n"
        << "--completion-zsh        output a zsh complete command you can eval\n"
        << "--password=password     specify a password for accessing encrypted files\n"
        << "--verbose               provide additional informational output\n"
        << "--progress              give progress indicators while writing output\n"
        << "--no-warn               suppress warnings\n"
        << "--linearize             generated a linearized (web optimized) file\n"
        << "--copy-encryption=file  copy encryption parameters from specified file\n"
        << "--encryption-file-password=password\n"
        << "                        password used to open the file from which encryption\n"
        << "                        parameters are being copied\n"
        << "--encrypt options --    generate an encrypted file\n"
        << "--decrypt               remove any encryption on the file\n"
        << "--password-is-hex-key   treat primary password option as a hex-encoded key\n"
        << "--suppress-password-recovery\n"
        << "                        do not attempt recovering from password string\n"
        << "                        encoding errors\n"
        << "--password-mode=mode    control qpdf's encoding of passwords\n"
        << "--pages options --      select specific pages from one or more files\n"
        << "--collate               causes files specified in --pages to be collated\n"
        << "                        rather than concatenated\n"
        << "--rotate=[+|-]angle[:page-range]\n"
        << "                        rotate each specified page 90, 180, or 270 degrees;\n"
        << "                        rotate all pages if no page range is given\n"
        << "--split-pages=[n]       write each output page to a separate file\n"
        << "--overlay options --    overlay pages from another file\n"
        << "--underlay options --   underlay pages from another file\n"
        << "\n"
        << "Note that you can use the @filename or @- syntax for any argument at any\n"
        << "point in the command. This provides a good way to specify a password without\n"
        << "having to explicitly put it on the command line.\n"
        << "\n"
        << "If none of --copy-encryption, --encrypt or --decrypt are given, qpdf will\n"
        << "preserve any encryption data associated with a file.\n"
        << "\n"
        << "Note that when copying encryption parameters from another file, all\n"
        << "parameters will be copied, including both user and owner passwords, even\n"
        << "if the user password is used to open the other file.  This works even if\n"
        << "the owner password is not known.\n"
        << "\n"
        << "The --password-is-hex-key option overrides the normal computation of\n"
        << "encryption keys. It only applies to the password used to open the main\n"
        << "file. This option is not ordinarily useful but can be helpful for forensic\n"
        << "or investigatory purposes. See manual for further discussion.\n"
        << "\n"
        << "The --rotate flag can be used to specify pages to rotate pages either\n"
        << "90, 180, or 270 degrees. The page range is specified in the same\n"
        << "format as with the --pages option, described below. Repeat the option\n"
        << "to rotate multiple groups of pages. If the angle is preceded by + or -,\n"
        << "it is added to or subtracted from the original rotation. Otherwise, the\n"
        << "rotation angle is set explicitly to the given value.\n"
        << "\n"
        << "If --split-pages is specified, each page is written to a separate output\n"
        << "file. File names are generated as follows:\n"
        << "* If the string %d appears in the output file name, it is replaced with a\n"
        << "  zero-padded page range starting from 1\n"
        << "* Otherwise, if the output file name ends in .pdf (case insensitive), a\n"
        << "  zero-padded page range, preceded by a dash, is inserted before the file\n"
        << "  extension\n"
        << "* Otherwise, the file name is appended with a zero-padded page range\n"
        << "  preceded by a dash.\n"
        << "Page ranges are single page numbers for single-page groups or first-last\n"
        << "for multipage groups.\n"
        << "\n"
        << "\n"
        << "Encryption Options\n"
        << "------------------\n"
        << "\n"
        << "  --encrypt user-password owner-password key-length flags --\n"
        << "\n"
        << "Note that -- terminates parsing of encryption flags.\n"
        << "\n"
        << "Either or both of the user password and the owner password may be\n"
        << "empty strings.\n"
        << "\n"
        << "key-length may be 40, 128, or 256\n"
        << "\n"
        << "Additional flags are dependent upon key length.\n"
        << "\n"
        << "  If 40:\n"
        << "\n"
        << "    --print=[yn]             allow printing\n"
        << "    --modify=[yn]            allow document modification\n"
        << "    --extract=[yn]           allow text/graphic extraction\n"
        << "    --annotate=[yn]          allow comments and form fill-in and signing\n"
        << "\n"
        << "  If 128:\n"
        << "\n"
        << "    --accessibility=[yn]     allow accessibility to visually impaired\n"
        << "    --extract=[yn]           allow other text/graphic extraction\n"
        << "    --print=print-opt        control printing access\n"
        << "    --assemble=[yn]          allow document assembly\n"
        << "    --annotate=[yn]          allow commenting/filling form fields\n"
        << "    --form=[yn]              allow filling form fields\n"
        << "    --modify-other=[yn]      allow other modifications\n"
        << "    --modify=modify-opt      control modify access (old way)\n"
        << "    --cleartext-metadata     prevents encryption of metadata\n"
        << "    --use-aes=[yn]           indicates whether to use AES encryption\n"
        << "    --force-V4               forces use of V=4 encryption handler\n"
        << "\n"
        << "  If 256, options are the same as 128 with these exceptions:\n"
        << "    --force-V4               this option is not available with 256-bit keys\n"
        << "    --use-aes                this option is always on with 256-bit keys\n"
        << "    --force-R5               forces use of deprecated R=5 encryption\n"
        << "\n"
        << "    print-opt may be:\n"
        << "\n"
        << "      full                  allow full printing\n"
        << "      low                   allow only low-resolution printing\n"
        << "      none                  disallow printing\n"
        << "\n"
        << "    modify-opt may be:\n"
        << "\n"
        << "      all                   allow full document modification\n"
        << "      annotate              allow comment authoring and form operations\n"
        << "      form                  allow form field fill-in and signing\n"
        << "      assembly              allow document assembly only\n"
        << "      none                  allow no modifications\n"
        << "\n"
        << "The default for each permission option is to be fully permissive. Please\n"
        << "refer to the manual for more details on the modify options.\n"
        << "\n"
        << "Specifying cleartext-metadata forces the PDF version to at least 1.5.\n"
        << "Specifying use of AES forces the PDF version to at least 1.6.  These\n"
        << "options are both off by default.\n"
        << "\n"
        << "The --force-V4 flag forces the V=4 encryption handler introduced in PDF 1.5\n"
        << "to be used even if not otherwise needed.  This option is primarily useful\n"
        << "for testing qpdf and has no other practical use.\n"
        << "\n"
        << "\n"
        << "Password Modes\n"
        << "----------------------\n"
        << "\n"
        << "The --password-mode controls how qpdf interprets passwords supplied\n"
        << "on the command-line. qpdf's default behavior is correct in almost all\n"
        << "cases, but you can fine-tune with this option.\n"
        << "\n"
        << "  bytes: use the password literally as supplied\n"
        << "  hex-bytes: interpret the password as a hex-encoded byte string\n"
        << "  unicode: interpret the password as a UTF-8 encoded string\n"
        << "  auto: attempt to infer the encoding and adjust as needed\n"
        << "\n"
        << "This is a complex topic. See the manual for a complete discussion.\n"
        << "\n"
        << "\n"
        << "Page Selection Options\n"
        << "----------------------\n"
        << "\n"
        << "These options allow pages to be selected from one or more PDF files.\n"
        << "Whatever file is given as the primary input file is used as the\n"
        << "starting point, but its pages are replaced with pages as specified.\n"
        << "\n"
        << "--keep-files-open=[yn]\n"
        << "--pages file [ --password=password ] [ page-range ] ... --\n"
        << "\n"
        << "For each file that pages should be taken from, specify the file, a\n"
        << "password needed to open the file (if any), and a page range.  The\n"
        << "password needs to be given only once per file.  If any of the input\n"
        << "files are the same as the primary input file or the file used to copy\n"
        << "encryption parameters (if specified), you do not need to repeat the\n"
        << "password here.  The same file can be repeated multiple times.  The\n"
        << "filename \".\" may be used to refer to the current input file.  All\n"
        << "non-page data (info, outlines, page numbers, etc. are taken from the\n"
        << "primary input file.  To discard this, use --empty as the primary\n"
        << "input.\n"
        << "\n"
        << "By default, when more than 200 distinct files are specified, qpdf will\n"
        << "close each file when not being referenced. With 200 files or fewer, all\n"
        << "files will be kept open at the same time. This behavior can be overridden\n"
        << "by specifying --keep-files-open=[yn]. Closing and opening files can have\n"
        << "very high overhead on certain file systems, especially networked file\n"
        << "systems.\n"
        << "\n"
        << "The page range is a set of numbers separated by commas, ranges of\n"
        << "numbers separated dashes, or combinations of those.  The character\n"
        << "\"z\" represents the last page.  A number preceded by an \"r\" indicates\n"
        << "to count from the end, so \"r3-r1\" would be the last three pages of the\n"
        << "document.  Pages can appear in any order.  Ranges can appear with a\n"
        << "high number followed by a low number, which causes the pages to appear in\n"
        << "reverse.  Repeating a number will cause an error, but the manual discusses\n"
        << "a workaround should you really want to include the same page twice.\n"
        << "\n"
        << "If the page range is omitted, the range of 1-z is assumed.  qpdf decides\n"
        << "that the page range is omitted if the range argument is either -- or a\n"
        << "valid file name and not a valid range.\n"
        << "\n"
        << "The usual behavior of --pages is to add all pages from the first file,\n"
        << "then all pages from the second file, and so on. If the --collate option\n"
        << "is specified, then pages are collated instead. In other words, qpdf takes\n"
        << "the first page from the first file, the first page from the second file,\n"
        << "and so on until it runs out of files; then it takes the second page from\n"
        << "each file, etc. When a file runs out of pages, it is skipped until all\n"
        << "specified pages are taken from all files.\n"
        << "\n"
        << "See the manual for examples and a discussion of additional subtleties.\n"
        << "\n"
        << "\n"
        << "Overlay and Underlay Options\n"
        << "-------------------------------\n"
        << "\n"
        << "These options allow pages from another file to be overlaid or underlaid\n"
        << "on the primary output. Overlaid pages are drawn on top of the destination\n"
        << "page and may obscure the page. Underlaid pages are drawn below the\n"
        << "destination page.\n"
        << "\n"
        << "{--overlay | --underlay } file\n"
        "      [ --password=password ]\n"
        "      [ --to=page-range ]\n"
        "      [ --from=[page-range] ]\n"
        "      [ --repeat=page-range ]\n"
        "      --\n"
        << "\n"
        << "For overlay and underlay, a file and optional password are specified, along\n"
        << "with a series of optional page ranges. The default behavior is that each\n"
        << "page of the overlay or underlay file is imposed on the corresponding page\n"
        << "of the primary output until it runs out of pages, and any extra pages are\n"
        << "ignored. The page range options all take page ranges in the same form as\n"
        << "the --pages option. They have the following meanings:\n"
        << "\n"
        << "  --to:     the pages in the primary output to which overlay/underlay is\n"
        << "            applied\n"
        << "  --from:   the pages from the overlay/underlay file that are used\n"
        << "  --repeat: pages from the overlay/underlay that are repeated after\n"
        << "            any \"from\" pages have been exhausted\n"
        << "\n"
        << "\n"
        << "Advanced Parsing Options\n"
        << "-------------------------------\n"
        << "\n"
        << "These options control aspects of how qpdf reads PDF files. Mostly these are\n"
        << "of use to people who are working with damaged files. There is little reason\n"
        << "to use these options unless you are trying to solve specific problems.\n"
        << "\n"
        << "--suppress-recovery       prevents qpdf from attempting to recover damaged files\n"
        << "--ignore-xref-streams     tells qpdf to ignore any cross-reference streams\n"
        << "\n"
        << "\n"
        << "Advanced Transformation Options\n"
        << "-------------------------------\n"
        << "\n"
        << "These transformation options control fine points of how qpdf creates\n"
        << "the output file.  Mostly these are of use only to people who are very\n"
        << "familiar with the PDF file format or who are PDF developers.\n"
        << "\n"
        << "--stream-data=option      controls transformation of stream data (below)\n"
        << "--compress-streams=[yn]   controls whether to compress streams on output\n"
        << "--decode-level=option     controls how to filter streams from the input\n"
        << "--normalize-content=[yn]  enables or disables normalization of content streams\n"
        << "--object-streams=mode     controls handing of object streams\n"
        << "--preserve-unreferenced   preserve unreferenced objects\n"
        << "--preserve-unreferenced-resources\n"
        << "                          preserve unreferenced page resources\n"
        << "--newline-before-endstream  always put a newline before endstream\n"
        << "--coalesce-contents       force all pages' content to be a single stream\n"
        << "--flatten-annotations=option\n"
        << "                          incorporate rendering of annotations into page\n"
        << "                          contents including those for interactive form\n"
        << "                          fields; may also want --generate-appearances\n"
        << "--generate-appearances    generate appearance streams for form fields\n"
        << "--optimize-images         compress images with DCT (JPEG) when advantageous\n"
        << "--oi-min-width=w          do not optimize images whose width is below w;\n"
        << "                          default is 128. Use 0 to mean no minimum\n"
        << "--oi-min-height=h         do not optimize images whose height is below h\n"
        << "                          default is 128. Use 0 to mean no minimum\n"
        << "--oi-min-area=a           do not optimize images whose pixel count is below a\n"
        << "                          default is 16,384. Use 0 to mean no minimum\n"
        << "--externalize-inline-images  convert inline images to regular images; by\n"
        << "                          default, images of at least 1,024 bytes are\n"
        << "                          externalized\n"
        << "--ii-min-bytes=bytes      specify minimum size of inline images to be\n"
        << "                          converted to regular images\n"
        << "--keep-inline-images      exclude inline images from image optimization\n"
        << "--qdf                     turns on \"QDF mode\" (below)\n"
        << "--linearize-pass1=file    write intermediate pass of linearized file\n"
        << "                          for debugging\n"
        << "--min-version=version     sets the minimum PDF version of the output file\n"
        << "--force-version=version   forces this to be the PDF version of the output file\n"
        << "\n"
        << "Options for --flatten-annotations are all, print, or screen. If the option\n"
        << "is print, only annotations marked as print are included. If the option is\n"
        << "screen, options marked as \"no view\" are excluded. Otherwise, annotations\n"
        << "are flattened regardless of the presence of print or NoView flags. It is\n"
        << "common for PDF files to have a flag set that appearance streams need to be\n"
        << "regenerated. This happens when someone changes a form value with software\n"
        << "that does not know how to render the new value. qpdf will not flatten form\n"
        << "fields in files like this. If you get this warning, you have two choices:\n"
        << "either use qpdf's --generate-appearances flag to tell qpdf to go ahead and\n"
        << "regenerate appearances, or use some other tool to generate the appearances.\n"
        << "qpdf does a pretty good job with most forms when only ASCII and \"Windows\n"
        << "ANSI\" characters are used in form field values, but if your form fields\n"
        << "contain other characters, rich text, or are other than left justified, you\n"
        << "will get better results first saving with other software.\n"
        << "\n"
        << "Version numbers may be expressed as major.minor.extension-level, so 1.7.3\n"
        << "means PDF version 1.7 at extension level 3.\n"
        << "\n"
        << "Values for stream data options:\n"
        << "\n"
        << "  compress              recompress stream data when possible (default)\n"
        << "  preserve              leave all stream data as is\n"
        << "  uncompress            uncompress stream data when possible\n"
        << "\n"
        << "Values for object stream mode:\n"
        << "\n"
        << "  preserve                  preserve original object streams (default)\n"
        << "  disable                   don't write any object streams\n"
        << "  generate                  use object streams wherever possible\n"
        << "\n"
        << "When --compress-streams=n is specified, this overrides the default behavior\n"
        << "of qpdf, which is to attempt compress uncompressed streams. Setting\n"
        << "stream data mode to uncompress or preserve has the same effect.\n"
        << "\n"
        << "The --decode-level parameter may be set to one of the following values:\n"
        << "  none              do not decode streams\n"
        << "  generalized       decode streams compressed with generalized filters\n"
        << "                    including LZW, Flate, and the ASCII encoding filters.\n"
        << "  specialized       additionally decode streams with non-lossy specialized\n"
        << "                    filters including RunLength\n"
        << "  all               additionally decode streams with lossy filters\n"
        << "                    including DCT (JPEG)\n"
        << "\n"
        << "In qdf mode, by default, content normalization is turned on, and the\n"
        << "stream data mode is set to uncompress.\n"
        << "\n"
        << "Setting the minimum PDF version of the output file may raise the version\n"
        << "but will never lower it.  Forcing the PDF version of the output file may\n"
        << "set the PDF version to a lower value than actually allowed by the file's\n"
        << "contents.  You should only do this if you have no other possible way to\n"
        << "open the file or if you know that the file definitely doesn't include\n"
        << "features not supported later versions.\n"
        << "\n"
        << "Testing, Inspection, and Debugging Options\n"
        << "------------------------------------------\n"
        << "\n"
        << "These options can be useful for digging into PDF files or for use in\n"
        << "automated test suites for software that uses the qpdf library.\n"
        << "\n"
        << "--deterministic-id        generate deterministic /ID\n"
        << "--static-id               generate static /ID: FOR TESTING ONLY!\n"
        << "--static-aes-iv           use a static initialization vector for AES-CBC\n"
        << "                          This is option is not secure!  FOR TESTING ONLY!\n"
        << "--no-original-object-ids  suppress original object ID comments in qdf mode\n"
        << "--show-encryption         quickly show encryption parameters\n"
        << "--show-encryption-key     when showing encryption, reveal the actual key\n"
        << "--check-linearization     check file integrity and linearization status\n"
        << "--show-linearization      check and show all linearization data\n"
        << "--show-xref               show the contents of the cross-reference table\n"
        << "--show-object=trailer|obj[,gen]\n"
        << "                          show the contents of the given object\n"
        << "  --raw-stream-data       show raw stream data instead of object contents\n"
        << "  --filtered-stream-data  show filtered stream data instead of object contents\n"
        << "--show-npages             print the number of pages in the file\n"
        << "--show-pages              shows the object/generation number for each page\n"
        << "  --with-images           also shows the object IDs for images on each page\n"
        << "--check                   check file structure + encryption, linearization\n"
        << "--json                    generate a json representation of the file\n"
        << "--json-help               describe the format of the json representation\n"
        << "--json-key=key            repeatable; prune json structure to include only\n"
        << "                          specified keys. If absent, all keys are shown\n"
        << "--json-object=trailer|[obj,gen]\n"
        << "                          repeatable; include only specified objects in the\n"
        << "                          \"objects\" section of the json. If absent, all\n"
        << "                          objects are shown\n"
        << "\n"
        << "The json representation generated by qpdf is designed to facilitate\n"
        << "processing of qpdf from other programming languages that have a hard\n"
        << "time calling C++ APIs. Run qpdf --json-help for details on the format.\n"
        << "The manual has more in-depth information about the json representation\n"
        << "and certain compatibility guarantees that qpdf provides.\n"
        << "\n"
        << "The --raw-stream-data and --filtered-stream-data options are ignored\n"
        << "unless --show-object is given.  Either of these options will cause the\n"
        << "stream data to be written to standard output.\n"
        << "\n"
        << "If --filtered-stream-data is given and --normalize-content=y is also\n"
        << "given, qpdf will attempt to normalize the stream data as if it is a\n"
        << "page content stream.  This attempt will be made even if it is not a\n"
        << "page content stream, in which case it will produce unusable results.\n"
        << "\n"
        << "Ordinarily, qpdf exits with a status of 0 on success or a status of 2\n"
        << "if any errors occurred.  If there were warnings but not errors, qpdf\n"
        << "exits with a status of 3. If warnings would have been issued but --no-warn\n"
        << "was given, an exit status of 3 is still used.\n";
}

void
ArgParser::argCompletionBash()
{
    std::cout << "complete -o bashdefault -o default -o nospace"
              << " -C " << argv[0] << " " << whoami << std::endl;
    // Put output before error so calling from zsh works properly
    std::string path = argv[0];
    size_t slash = path.find('/');
    if ((slash != 0) && (slash != std::string::npos))
    {
        std::cerr << "WARNING: qpdf completion enabled"
                  << " using relative path to qpdf" << std::endl;
    }
}

void
ArgParser::argCompletionZsh()
{
    std::cout << "autoload -U +X bashcompinit && bashcompinit && ";
    argCompletionBash();
}
void
ArgParser::argJsonHelp()
{
    // Make sure the output looks right on an 80-column display.
    //               1         2         3         4         5         6         7         8
    //      12345678901234567890123456789012345678901234567890123456789012345678901234567890
    std::cout
        << "The json block below contains the same structure with the same keys as the"
        << std::endl
        << "json generated by qpdf. In the block below, the values are descriptions of"
        << std::endl
        << "the meanings of those entries. The specific contract guaranteed by qpdf in"
        << std::endl
        << "its json representation is explained in more detail in the manual. You can"
        << std::endl
        << "specify a subset of top-level keys when you invoke qpdf, but the \"version\""
        << std::endl
        << "and \"parameters\" keys will always be present."
        << std::endl
        << std::endl
        << json_schema().unparse()
        << std::endl;
}

void
ArgParser::argPassword(char* parameter)
{
    o.password = parameter;
}

void
ArgParser::argEmpty()
{
    o.infilename = "";
}

void
ArgParser::argLinearize()
{
    o.linearize = true;
}

void
ArgParser::argEncrypt()
{
    ++cur_arg;
    if (cur_arg + 3 > argc)
    {
        if (this->bash_completion)
        {
            if (cur_arg == argc)
            {
                this->completions.insert("user-password");
            }
            else if (cur_arg + 1 == argc)
            {
                this->completions.insert("owner-password");
            }
            else if (cur_arg + 2 == argc)
            {
                this->completions.insert("40");
                this->completions.insert("128");
                this->completions.insert("256");
            }
            return;
        }
        else
        {
            usage("insufficient arguments to --encrypt");
        }
    }
    o.user_password = argv[cur_arg++];
    o.owner_password = argv[cur_arg++];
    std::string len_str = argv[cur_arg];
    if (len_str == "40")
    {
	o.keylen = 40;
        this->option_table = &(this->encrypt40_option_table);
    }
    else if (len_str == "128")
    {
	o.keylen = 128;
        this->option_table = &(this->encrypt128_option_table);
    }
    else if (len_str == "256")
    {
	o.keylen = 256;
        o.use_aes = true;
        this->option_table = &(this->encrypt256_option_table);
    }
    else
    {
	usage("encryption key length must be 40, 128, or 256");
    }
}

void
ArgParser::argDecrypt()
{
    o.decrypt = true;
    o.encrypt = false;
    o.copy_encryption = false;
}

void
ArgParser::argPasswordIsHexKey()
{
    o.password_is_hex_key = true;
}

void
ArgParser::argSuppressPasswordRecovery()
{
    o.suppress_password_recovery = true;
}

void
ArgParser::argPasswordMode(char* parameter)
{
    if (strcmp(parameter, "bytes") == 0)
    {
        o.password_mode = pm_bytes;
    }
    else if (strcmp(parameter, "hex-bytes") == 0)
    {
        o.password_mode = pm_hex_bytes;
    }
    else if (strcmp(parameter, "unicode") == 0)
    {
        o.password_mode = pm_unicode;
    }
    else if (strcmp(parameter, "auto") == 0)
    {
        o.password_mode = pm_auto;
    }
    else
    {
        usage("invalid password-mode option");
    }
}

void
ArgParser::argCopyEncryption(char* parameter)
{
    o.encryption_file = parameter;
    o.copy_encryption = true;
    o.encrypt = false;
    o.decrypt = false;
}

void
ArgParser::argEncryptionFilePassword(char* parameter)
{
    o.encryption_file_password = parameter;
}

void
ArgParser::argCollate()
{
    o.collate = true;
}

void
ArgParser::argPages()
{
    ++cur_arg;
    o.page_specs = parsePagesOptions();
    if (o.page_specs.empty())
    {
        usage("--pages: no page specifications given");
    }
}

void
ArgParser::argUnderlay()
{
    parseUnderOverlayOptions(&o.underlay);
}

void
ArgParser::argOverlay()
{
    parseUnderOverlayOptions(&o.overlay);
}

void
ArgParser::argRotate(char* parameter)
{
    parseRotationParameter(parameter);
}

void
ArgParser::argStreamData(char* parameter)
{
    o.stream_data_set = true;
    if (strcmp(parameter, "compress") == 0)
    {
        o.stream_data_mode = qpdf_s_compress;
    }
    else if (strcmp(parameter, "preserve") == 0)
    {
        o.stream_data_mode = qpdf_s_preserve;
    }
    else if (strcmp(parameter, "uncompress") == 0)
    {
        o.stream_data_mode = qpdf_s_uncompress;
    }
    else
    {
        // If this happens, it means streamDataChoices in
        // ArgParser::initOptionTable is wrong.
        usage("invalid stream-data option");
    }
}

void
ArgParser::argCompressStreams(char* parameter)
{
    o.compress_streams_set = true;
    o.compress_streams = (strcmp(parameter, "y") == 0);
}

void
ArgParser::argDecodeLevel(char* parameter)
{
    o.decode_level_set = true;
    if (strcmp(parameter, "none") == 0)
    {
        o.decode_level = qpdf_dl_none;
    }
    else if (strcmp(parameter, "generalized") == 0)
    {
        o.decode_level = qpdf_dl_generalized;
    }
    else if (strcmp(parameter, "specialized") == 0)
    {
        o.decode_level = qpdf_dl_specialized;
    }
    else if (strcmp(parameter, "all") == 0)
    {
        o.decode_level = qpdf_dl_all;
    }
    else
    {
        // If this happens, it means decodeLevelChoices in
        // ArgParser::initOptionTable is wrong.
        usage("invalid option");
    }
}

void
ArgParser::argNormalizeContent(char* parameter)
{
    o.normalize_set = true;
    o.normalize = (strcmp(parameter, "y") == 0);
}

void
ArgParser::argSuppressRecovery()
{
    o.suppress_recovery = true;
}

void
ArgParser::argObjectStreams(char* parameter)
{
    o.object_stream_set = true;
    if (strcmp(parameter, "disable") == 0)
    {
        o.object_stream_mode = qpdf_o_disable;
    }
    else if (strcmp(parameter, "preserve") == 0)
    {
        o.object_stream_mode = qpdf_o_preserve;
    }
    else if (strcmp(parameter, "generate") == 0)
    {
        o.object_stream_mode = qpdf_o_generate;
    }
    else
    {
        // If this happens, it means objectStreamsChoices in
        // ArgParser::initOptionTable is wrong.
        usage("invalid object stream mode");
    }
}

void
ArgParser::argIgnoreXrefStreams()
{
    o.ignore_xref_streams = true;
}

void
ArgParser::argQdf()
{
    o.qdf_mode = true;
}

void
ArgParser::argPreserveUnreferenced()
{
    o.preserve_unreferenced_objects = true;
}

void
ArgParser::argPreserveUnreferencedResources()
{
    o.preserve_unreferenced_page_resources = true;
}

void
ArgParser::argKeepFilesOpen(char* parameter)
{
    o.keep_files_open_set = true;
    o.keep_files_open = (strcmp(parameter, "y") == 0);
}

void
ArgParser::argNewlineBeforeEndstream()
{
    o.newline_before_endstream = true;
}

void
ArgParser::argLinearizePass1(char* parameter)
{
    o.linearize_pass1 = parameter;
}

void
ArgParser::argCoalesceContents()
{
    o.coalesce_contents = true;
}

void
ArgParser::argFlattenAnnotations(char* parameter)
{
    o.flatten_annotations = true;
    if (strcmp(parameter, "screen") == 0)
    {
        o.flatten_annotations_forbidden |= an_no_view;
    }
    else if (strcmp(parameter, "print") == 0)
    {
        o.flatten_annotations_required |= an_print;
    }
}

void
ArgParser::argGenerateAppearances()
{
    o.generate_appearances = true;
}

void
ArgParser::argMinVersion(char* parameter)
{
    o.min_version = parameter;
}

void
ArgParser::argForceVersion(char* parameter)
{
    o.force_version = parameter;
}

void
ArgParser::argSplitPages(char* parameter)
{
    int n = ((parameter == 0) ? 1 :
             QUtil::string_to_int(parameter));
    o.split_pages = n;
}

void
ArgParser::argVerbose()
{
    o.verbose = true;
}

void
ArgParser::argProgress()
{
    o.progress = true;
}

void
ArgParser::argNoWarn()
{
    o.suppress_warnings = true;
}

void
ArgParser::argDeterministicId()
{
    o.deterministic_id = true;
}

void
ArgParser::argStaticId()
{
    o.static_id = true;
}

void
ArgParser::argStaticAesIv()
{
    o.static_aes_iv = true;
}

void
ArgParser::argNoOriginalObjectIds()
{
    o.suppress_original_object_id = true;
}

void
ArgParser::argShowEncryption()
{
    o.show_encryption = true;
    o.require_outfile = false;
}

void
ArgParser::argShowEncryptionKey()
{
    o.show_encryption_key = true;
}

void
ArgParser::argCheckLinearization()
{
    o.check_linearization = true;
    o.require_outfile = false;
}

void
ArgParser::argShowLinearization()
{
    o.show_linearization = true;
    o.require_outfile = false;
}

void
ArgParser::argShowXref()
{
    o.show_xref = true;
    o.require_outfile = false;
}

void
ArgParser::argShowObject(char* parameter)
{
    parse_object_id(parameter, o.show_trailer, o.show_obj, o.show_gen);
    o.require_outfile = false;
}

void
ArgParser::argRawStreamData()
{
    o.show_raw_stream_data = true;
}

void
ArgParser::argFilteredStreamData()
{
    o.show_filtered_stream_data = true;
}

void
ArgParser::argShowNpages()
{
    o.show_npages = true;
    o.require_outfile = false;
}

void
ArgParser::argShowPages()
{
    o.show_pages = true;
    o.require_outfile = false;
}

void
ArgParser::argWithImages()
{
    o.show_page_images = true;
}

void
ArgParser::argJson()
{
    o.json = true;
    o.require_outfile = false;
}

void
ArgParser::argJsonKey(char* parameter)
{
    o.json_keys.insert(parameter);
}

void
ArgParser::argJsonObject(char* parameter)
{
    o.json_objects.insert(parameter);
}

void
ArgParser::argCheck()
{
    o.check = true;
    o.require_outfile = false;
}

void
ArgParser::argOptimizeImages()
{
    o.optimize_images = true;
}

void
ArgParser::argExternalizeInlineImages()
{
    o.externalize_inline_images = true;
}

void
ArgParser::argKeepInlineImages()
{
    o.keep_inline_images = true;
}

void
ArgParser::argOiMinWidth(char* parameter)
{
    o.oi_min_width = QUtil::string_to_int(parameter);
}

void
ArgParser::argOiMinHeight(char* parameter)
{
    o.oi_min_height = QUtil::string_to_int(parameter);
}

void
ArgParser::argOiMinArea(char* parameter)
{
    o.oi_min_area = QUtil::string_to_int(parameter);
}

void
ArgParser::argIiMinBytes(char* parameter)
{
    o.ii_min_bytes = QUtil::string_to_int(parameter);
}

void
ArgParser::arg40Print(char* parameter)
{
    o.r2_print = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg40Modify(char* parameter)
{
    o.r2_modify = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg40Extract(char* parameter)
{
    o.r2_extract = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg40Annotate(char* parameter)
{
    o.r2_annotate = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128Accessibility(char* parameter)
{
    o.r3_accessibility = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128Extract(char* parameter)
{
    o.r3_extract = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128Print(char* parameter)
{
    if (strcmp(parameter, "full") == 0)
    {
        o.r3_print = qpdf_r3p_full;
    }
    else if (strcmp(parameter, "low") == 0)
    {
        o.r3_print = qpdf_r3p_low;
    }
    else if (strcmp(parameter, "none") == 0)
    {
        o.r3_print = qpdf_r3p_none;
    }
    else
    {
        usage("invalid print option");
    }
}

void
ArgParser::arg128Modify(char* parameter)
{
    if (strcmp(parameter, "all") == 0)
    {
        o.r3_assemble = true;
        o.r3_annotate_and_form = true;
        o.r3_form_filling = true;
        o.r3_modify_other = true;
    }
    else if (strcmp(parameter, "annotate") == 0)
    {
        o.r3_assemble = true;
        o.r3_annotate_and_form = true;
        o.r3_form_filling = true;
        o.r3_modify_other = false;
    }
    else if (strcmp(parameter, "form") == 0)
    {
        o.r3_assemble = true;
        o.r3_annotate_and_form = false;
        o.r3_form_filling = true;
        o.r3_modify_other = false;
    }
    else if (strcmp(parameter, "assembly") == 0)
    {
        o.r3_assemble = true;
        o.r3_annotate_and_form = false;
        o.r3_form_filling = false;
        o.r3_modify_other = false;
    }
    else if (strcmp(parameter, "none") == 0)
    {
        o.r3_assemble = false;
        o.r3_annotate_and_form = false;
        o.r3_form_filling = false;
        o.r3_modify_other = false;
    }
    else
    {
        usage("invalid modify option");
    }
}

void
ArgParser::arg128ClearTextMetadata()
{
    o.cleartext_metadata = true;
}

void
ArgParser::arg128Assemble(char* parameter)
{
    o.r3_assemble = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128Annotate(char* parameter)
{
    o.r3_annotate_and_form = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128Form(char* parameter)
{
    o.r3_form_filling = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128ModOther(char* parameter)
{
    o.r3_modify_other = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128UseAes(char* parameter)
{
    o.use_aes = (strcmp(parameter, "y") == 0);
}

void
ArgParser::arg128ForceV4()
{
    o.force_V4 = true;
}

void
ArgParser::arg256ForceR5()
{
    o.force_R5 = true;
}

void
ArgParser::argEndEncrypt()
{
    o.encrypt = true;
    o.decrypt = false;
    o.copy_encryption = false;
    this->option_table = &(this->main_option_table);
}

void
ArgParser::argUOpositional(char* arg)
{
    if (o.under_overlay->filename)
    {
        usage(o.under_overlay->which + " file already specified");
    }
    else
    {
        o.under_overlay->filename = arg;
    }
}

void
ArgParser::argUOto(char* parameter)
{
    parseNumrange(parameter, 0);
    o.under_overlay->to_nr = parameter;
}

void
ArgParser::argUOfrom(char* parameter)
{
    if (strlen(parameter))
    {
        parseNumrange(parameter, 0);
    }
    o.under_overlay->from_nr = parameter;
}

void
ArgParser::argUOrepeat(char* parameter)
{
    if (strlen(parameter))
    {
        parseNumrange(parameter, 0);
    }
    o.under_overlay->repeat_nr = parameter;
}

void
ArgParser::argUOpassword(char* parameter)
{
    o.under_overlay->password = parameter;
}

void
ArgParser::argEndUnderOverlay()
{
    this->option_table = &(this->main_option_table);
    if (0 == o.under_overlay->filename)
    {
        usage(o.under_overlay->which + " file not specified");
    }
    o.under_overlay = 0;
}

void
ArgParser::handleArgFileArguments()
{
    // Support reading arguments from files. Create a new argv. Ensure
    // that argv itself as well as all its contents are automatically
    // deleted by using PointerHolder objects to back the pointers in
    // argv.
    new_argv.push_back(PointerHolder<char>(true, QUtil::copy_string(argv[0])));
    for (int i = 1; i < argc; ++i)
    {
        char* argfile = 0;
        if ((strlen(argv[i]) > 1) && (argv[i][0] == '@'))
        {
            try
            {
                argfile = 1 + argv[i];
                if (strcmp(argfile, "-") != 0)
                {
                    fclose(QUtil::safe_fopen(argfile, "rb"));
                }
            }
            catch (std::runtime_error&)
            {
                // The file's not there; treating as regular option
                argfile = 0;
            }
        }
        if (argfile)
        {
            readArgsFromFile(1+argv[i]);
        }
        else
        {
            new_argv.push_back(
                PointerHolder<char>(true, QUtil::copy_string(argv[i])));
        }
    }
    argv_ph = PointerHolder<char*>(true, new char*[1+new_argv.size()]);
    argv = argv_ph.getPointer();
    for (size_t i = 0; i < new_argv.size(); ++i)
    {
        argv[i] = new_argv.at(i).getPointer();
    }
    argc = static_cast<int>(new_argv.size());
    argv[argc] = 0;
}

void
ArgParser::handleBashArguments()
{
    // Do a minimal job of parsing bash_line into arguments. This
    // doesn't do everything the shell does (e.g. $(...), variable
    // expansion, arithmetic, globs, etc.), but it should be good
    // enough for purposes of handling completion. As we build up the
    // new argv, we can't use this->new_argv because this code has to
    // interoperate with @file arguments, so memory for both ways of
    // fabricating argv has to be protected.

    bool last_was_backslash = false;
    enum { st_top, st_squote, st_dquote } state = st_top;
    std::string arg;
    for (std::string::iterator iter = bash_line.begin();
         iter != bash_line.end(); ++iter)
    {
        char ch = (*iter);
        if (last_was_backslash)
        {
            arg.append(1, ch);
            last_was_backslash = false;
        }
        else if (ch == '\\')
        {
            last_was_backslash = true;
        }
        else
        {
            bool append = false;
            switch (state)
            {
              case st_top:
                if (QUtil::is_space(ch))
                {
                    if (! arg.empty())
                    {
                        bash_argv.push_back(
                            PointerHolder<char>(
                                true, QUtil::copy_string(arg.c_str())));
                        arg.clear();
                    }
                }
                else if (ch == '"')
                {
                    state = st_dquote;
                }
                else if (ch == '\'')
                {
                    state = st_squote;
                }
                else
                {
                    append = true;
                }
                break;

              case st_squote:
                if (ch == '\'')
                {
                    state = st_top;
                }
                else
                {
                    append = true;
                }
                break;

              case st_dquote:
                if (ch == '"')
                {
                    state = st_top;
                }
                else
                {
                    append = true;
                }
                break;
            }
            if (append)
            {
                arg.append(1, ch);
            }
        }
    }
    if (bash_argv.empty())
    {
        // This can't happen if properly invoked by bash, but ensure
        // we have a valid argv[0] regardless.
        bash_argv.push_back(
            PointerHolder<char>(
                true, QUtil::copy_string(argv[0])));
    }
    // Explicitly discard any non-space-terminated word. The "current
    // word" is handled specially.
    bash_argv_ph = PointerHolder<char*>(true, new char*[1+bash_argv.size()]);
    argv = bash_argv_ph.getPointer();
    for (size_t i = 0; i < bash_argv.size(); ++i)
    {
        argv[i] = bash_argv.at(i).getPointer();
    }
    argc = static_cast<int>(bash_argv.size());
    argv[argc] = 0;
}

void usageExit(std::string const& msg)
{
    std::cerr
	<< std::endl
	<< whoami << ": " << msg << std::endl
	<< std::endl
	<< "Usage: " << whoami << " [options] infile outfile" << std::endl
	<< "For detailed help, run " << whoami << " --help" << std::endl
	<< std::endl;
    exit(EXIT_ERROR);
}

void
ArgParser::usage(std::string const& message)
{
    if (this->bash_completion)
    {
        // This will cause bash to fall back to regular file completion.
        exit(0);
    }
    else
    {
        usageExit(message);
    }
}

static std::string show_bool(bool v)
{
    return v ? "allowed" : "not allowed";
}

static std::string show_encryption_method(QPDF::encryption_method_e method)
{
    std::string result = "unknown";
    switch (method)
    {
      case QPDF::e_none:
        result = "none";
        break;
      case QPDF::e_unknown:
        result = "unknown";
        break;
      case QPDF::e_rc4:
        result = "RC4";
        break;
      case QPDF::e_aes:
        result = "AESv2";
        break;
      case QPDF::e_aesv3:
        result = "AESv3";
        break;
        // no default so gcc will warn for missing case
    }
    return result;
}

static void show_encryption(QPDF& pdf, Options& o)
{
    // Extract /P from /Encrypt
    int R = 0;
    int P = 0;
    int V = 0;
    QPDF::encryption_method_e stream_method = QPDF::e_unknown;
    QPDF::encryption_method_e string_method = QPDF::e_unknown;
    QPDF::encryption_method_e file_method = QPDF::e_unknown;
    if (! pdf.isEncrypted(R, P, V,
                          stream_method, string_method, file_method))
    {
	std::cout << "File is not encrypted" << std::endl;
    }
    else
    {
	std::cout << "R = " << R << std::endl;
	std::cout << "P = " << P << std::endl;
	std::string user_password = pdf.getTrimmedUserPassword();
        std::string encryption_key = pdf.getEncryptionKey();
	std::cout << "User password = " << user_password << std::endl;
        if (o.show_encryption_key)
        {
            std::cout << "Encryption key = "
                      << QUtil::hex_encode(encryption_key) << std::endl;
        }
        std::cout << "extract for accessibility: "
		  << show_bool(pdf.allowAccessibility()) << std::endl
                  << "extract for any purpose: "
		  << show_bool(pdf.allowExtractAll()) << std::endl
                  << "print low resolution: "
		  << show_bool(pdf.allowPrintLowRes()) << std::endl
                  << "print high resolution: "
		  << show_bool(pdf.allowPrintHighRes()) << std::endl
                  << "modify document assembly: "
		  << show_bool(pdf.allowModifyAssembly()) << std::endl
                  << "modify forms: "
		  << show_bool(pdf.allowModifyForm()) << std::endl
                  << "modify annotations: "
		  << show_bool(pdf.allowModifyAnnotation()) << std::endl
                  << "modify other: "
		  << show_bool(pdf.allowModifyOther()) << std::endl
                  << "modify anything: "
		  << show_bool(pdf.allowModifyAll()) << std::endl;
        if (V >= 4)
        {
            std::cout << "stream encryption method: "
                      << show_encryption_method(stream_method) << std::endl
                      << "string encryption method: "
                      << show_encryption_method(string_method) << std::endl
                      << "file encryption method: "
                      << show_encryption_method(file_method) << std::endl;
        }
    }
}

std::vector<int>
ArgParser::parseNumrange(char const* range, int max, bool throw_error)
{
    try
    {
        return QUtil::parse_numrange(range, max);
    }
    catch (std::runtime_error& e)
    {
        if (throw_error)
        {
            throw(e);
        }
        else
        {
            usage(e.what());
        }
    }
    return std::vector<int>();
}

std::vector<PageSpec>
ArgParser::parsePagesOptions()
{
    std::vector<PageSpec> result;
    while (1)
    {
        if ((cur_arg < argc) && (strcmp(argv[cur_arg], "--") == 0))
        {
            break;
        }
        if (cur_arg + 1 >= argc)
        {
            usage("insufficient arguments to --pages");
        }
        char const* file = argv[cur_arg++];
        char const* password = 0;
        char const* range = argv[cur_arg++];
        if (strncmp(range, "--password=", 11) == 0)
        {
            // Oh, that's the password, not the range
            if (cur_arg + 1 >= argc)
            {
                usage("insufficient arguments to --pages");
            }
            password = range + 11;
            range = argv[cur_arg++];
        }

        // See if the user omitted the range entirely, in which case
        // we assume "1-z".
        bool range_omitted = false;
        if (strcmp(range, "--") == 0)
        {
            // The filename or password was the last argument
            QTC::TC("qpdf", "qpdf pages range omitted at end");
            range_omitted = true;
        }
        else
        {
            try
            {
                parseNumrange(range, 0, true);
            }
            catch (std::runtime_error& e1)
            {
                // The range is invalid.  Let's see if it's a file.
                try
                {
                    fclose(QUtil::safe_fopen(range, "rb"));
                    // Yup, it's a file.
                    QTC::TC("qpdf", "qpdf pages range omitted in middle");
                    range_omitted = true;
                }
                catch (std::runtime_error&)
                {
                    // Give the range error
                    usage(e1.what());
                }
            }
        }
        if (range_omitted)
        {
            --cur_arg;
            range = "1-z";
        }

        result.push_back(PageSpec(file, password, range));
    }
    return result;
}

void
ArgParser::parseUnderOverlayOptions(UnderOverlay* uo)
{
    o.under_overlay = uo;
    this->option_table = &(this->under_overlay_option_table);
}

QPDFPageData::QPDFPageData(std::string const& filename,
                           QPDF* qpdf,
                           char const* range) :
    filename(filename),
    qpdf(qpdf),
    orig_pages(qpdf->getAllPages())
{
    try
    {
        this->selected_pages =
            QUtil::parse_numrange(range, this->orig_pages.size());
    }
    catch (std::runtime_error& e)
    {
        usageExit("parsing numeric range for " + filename + ": " + e.what());
    }
}

QPDFPageData::QPDFPageData(QPDFPageData const& other, int page) :
    filename(other.filename),
    qpdf(other.qpdf),
    orig_pages(other.orig_pages)
{
    this->selected_pages.push_back(page);
}

static void parse_version(std::string const& full_version_string,
                          std::string& version, int& extension_level)
{
    PointerHolder<char> vp(true, QUtil::copy_string(full_version_string));
    char* v = vp.getPointer();
    char* p1 = strchr(v, '.');
    char* p2 = (p1 ? strchr(1 + p1, '.') : 0);
    if (p2 && *(p2 + 1))
    {
        *p2++ = '\0';
        extension_level = QUtil::string_to_int(p2);
    }
    version = v;
}

void
ArgParser::readArgsFromFile(char const* filename)
{
    std::list<std::string> lines;
    if (strcmp(filename, "-") == 0)
    {
        QTC::TC("qpdf", "qpdf read args from stdin");
        lines = QUtil::read_lines_from_file(std::cin);
    }
    else
    {
        QTC::TC("qpdf", "qpdf read args from file");
        lines = QUtil::read_lines_from_file(filename);
    }
    for (std::list<std::string>::iterator iter = lines.begin();
         iter != lines.end(); ++iter)
    {
        new_argv.push_back(
            PointerHolder<char>(true, QUtil::copy_string((*iter).c_str())));
    }
}

void
ArgParser::handleHelpArgs()
{
    // Handle special-case informational options that are only
    // available as the sole option.

    if (argc != 2)
    {
        return;
    }
    char* arg = argv[1];
    if (*arg != '-')
    {
        return;
    }
    ++arg;
    if (*arg == '-')
    {
        ++arg;
    }
    if (! *arg)
    {
        return;
    }
    if (this->help_option_table.count(arg))
    {
        (this->*(this->help_option_table[arg].bare_arg_handler))();
        exit(0);
    }
}

void
ArgParser::parseRotationParameter(std::string const& parameter)
{
    std::string angle_str;
    std::string range;
    size_t colon = parameter.find(':');
    int relative = 0;
    if (colon != std::string::npos)
    {
        if (colon > 0)
        {
            angle_str = parameter.substr(0, colon);
        }
        if (colon + 1 < parameter.length())
        {
            range = parameter.substr(colon + 1);
        }
    }
    else
    {
        angle_str = parameter;
    }
    if (angle_str.length() > 0)
    {
        char first = angle_str.at(0);
        if ((first == '+') || (first == '-'))
        {
            relative = ((first == '+') ? 1 : -1);
            angle_str = angle_str.substr(1);
        }
        else if (! QUtil::is_digit(angle_str.at(0)))
        {
            angle_str = "";
        }
    }
    if (range.empty())
    {
        range = "1-z";
    }
    bool range_valid = false;
    try
    {
        parseNumrange(range.c_str(), 0, true);
        range_valid = true;
    }
    catch (std::runtime_error const&)
    {
        // ignore
    }
    if (range_valid &&
        ((angle_str == "90") || (angle_str == "180") || (angle_str == "270")))
    {
        int angle = QUtil::string_to_int(angle_str.c_str());
        if (relative == -1)
        {
            angle = -angle;
        }
        o.rotations[range] = RotationSpec(angle, (relative != 0));
    }
    else
    {
        usage("invalid parameter to rotate: " + parameter);
    }
}

void
ArgParser::checkCompletion()
{
    // See if we're being invoked from bash completion.
    std::string bash_point_env;
    if (QUtil::get_env("COMP_LINE", &bash_line) &&
        QUtil::get_env("COMP_POINT", &bash_point_env))
    {
        int p = QUtil::string_to_int(bash_point_env.c_str());
        if ((p > 0) && (p <= static_cast<int>(bash_line.length())))
        {
            // Truncate the line. We ignore everything at or after the
            // cursor for completion purposes.
            bash_line = bash_line.substr(0, p);
        }
        // Set bash_cur and bash_prev based on bash_line rather than
        // relying on argv. This enables us to use bashcompinit to get
        // completion in zsh too since bashcompinit sets COMP_LINE and
        // COMP_POINT but doesn't invoke the command with options like
        // bash does.

        // p is equal to length of the string. Walk backwards looking
        // for the first separator. bash_cur is everything after the
        // last separator, possibly empty.
        char sep(0);
        while (--p > 0)
        {
            char ch = bash_line.at(p);
            if ((ch == ' ') || (ch == '=') || (ch == ':'))
            {
                sep = ch;
                break;
            }
        }
        bash_cur = bash_line.substr(1+p, std::string::npos);
        if ((sep == ':') || (sep == '='))
        {
            // Bash sets prev to the non-space separator if any.
            // Actually, if there are multiple separators in a row,
            // they are all included in prev, but that detail is not
            // important to us and not worth coding.
            bash_prev = bash_line.substr(p, 1);
        }
        else
        {
            // Go back to the last separator and set prev based on
            // that.
            int p1 = p;
            while (--p1 > 0)
            {
                char ch = bash_line.at(p1);
                if ((ch == ' ') || (ch == ':') || (ch == '='))
                {
                    bash_prev = bash_line.substr(p1 + 1, p - p1 - 1);
                    break;
                }
            }
        }
        if (bash_prev.empty())
        {
            bash_prev = bash_line.substr(0, p);
        }
        if (argc == 1)
        {
            // This is probably zsh using bashcompinit. There are a
            // few differences in the expected output.
            zsh_completion = true;
        }
        handleBashArguments();
        bash_completion = true;
    }
}

void
ArgParser::parseOptions()
{
    checkCompletion();
    if (! this->bash_completion)
    {
        handleHelpArgs();
    }
    handleArgFileArguments();
    for (cur_arg = 1; cur_arg < argc; ++cur_arg)
    {
        char* arg = argv[cur_arg];
        if (strcmp(arg, "--") == 0)
        {
            // Special case for -- option, which is used to break out
            // of subparsers.
            OptionEntry& oe = (*this->option_table)["--"];
            if (oe.bare_arg_handler)
            {
                (this->*(oe.bare_arg_handler))();
            }
        }
        else if ((arg[0] == '-') && (strcmp(arg, "-") != 0))
        {
            ++arg;
            if (arg[0] == '-')
            {
                // Be lax about -arg vs --arg
                ++arg;
            }
            char* parameter = 0;
            if (strlen(arg) > 0)
            {
                // Prevent --=something from being treated as an empty
                // arg since the empty string in the option table is
                // for positional arguments.
                parameter = const_cast<char*>(strchr(1 + arg, '='));
            }
            if (parameter)
            {
                *parameter++ = 0;
            }

            std::string arg_s(arg);
            if (arg_s.empty() ||
                (arg_s.at(0) == '-') ||
                (0 == this->option_table->count(arg_s)))
            {
                usage(std::string("unknown option --") + arg);
            }

            OptionEntry& oe = (*this->option_table)[arg_s];
            if ((oe.parameter_needed && (0 == parameter)) ||
                ((! oe.choices.empty() &&
                  ((0 == parameter) ||
                   (0 == oe.choices.count(parameter))))))
            {
                std::string message =
                    "--" + arg_s + " must be given as --" + arg_s + "=";
                if (! oe.choices.empty())
                {
                    QTC::TC("qpdf", "qpdf required choices");
                    message += "{";
                    for (std::set<std::string>::iterator iter =
                             oe.choices.begin();
                         iter != oe.choices.end(); ++iter)
                    {
                        if (iter != oe.choices.begin())
                        {
                            message += ",";
                        }
                        message += *iter;
                    }
                    message += "}";
                }
                else if (! oe.parameter_name.empty())
                {
                    QTC::TC("qpdf", "qpdf required parameter");
                    message += oe.parameter_name;
                }
                else
                {
                    // should not be possible
                    message += "option";
                }
                usage(message);
            }
            if (oe.bare_arg_handler)
            {
                (this->*(oe.bare_arg_handler))();
            }
            else if (oe.param_arg_handler)
            {
                (this->*(oe.param_arg_handler))(parameter);
            }
        }
        else if (0 != this->option_table->count(""))
        {
            // The empty string maps to the positional argument
            // handler.
            OptionEntry& oe = (*this->option_table)[""];
            if (oe.param_arg_handler)
            {
                (this->*(oe.param_arg_handler))(arg);
            }
        }
        else
        {
            usage(std::string("unknown argument ") + arg);
        }
    }
    if (this->bash_completion)
    {
        handleCompletion();
    }
    else
    {
        doFinalChecks();
    }
}

void
ArgParser::doFinalChecks()
{
    if (this->option_table != &(this->main_option_table))
    {
        usage("missing -- at end of options");
    }
    if (o.infilename == 0)
    {
        usage("an input file name is required");
    }
    else if (o.require_outfile && (o.outfilename == 0))
    {
        usage("an output file name is required; use - for standard output");
    }
    else if ((! o.require_outfile) && (o.outfilename != 0))
    {
        usage("no output file may be given for this option");
    }
    if (o.optimize_images && (! o.keep_inline_images))
    {
        o.externalize_inline_images = true;
    }

    if (o.require_outfile && (strcmp(o.outfilename, "-") == 0))
    {
        if (o.split_pages)
        {
            usage("--split-pages may not be used when"
                  " writing to standard output");
        }
        if (o.verbose)
        {
            usage("--verbose may not be used when"
                  " writing to standard output");
        }
        if (o.progress)
        {
            usage("--progress may not be used when"
                  " writing to standard output");
        }
    }

    if (QUtil::same_file(o.infilename, o.outfilename))
    {
        QTC::TC("qpdf", "qpdf same file error");
        usage("input file and output file are the same;"
              " this would cause input file to be lost");
    }
}

void
ArgParser::addChoicesToCompletions(std::string const& option,
                                   std::string const& extra_prefix)
{
    if (this->option_table->count(option) != 0)
    {
        OptionEntry& oe = (*this->option_table)[option];
        for (std::set<std::string>::iterator iter = oe.choices.begin();
             iter != oe.choices.end(); ++iter)
        {
            completions.insert(extra_prefix + *iter);
        }
    }
}

void
ArgParser::addOptionsToCompletions()
{
    for (std::map<std::string, OptionEntry>::iterator iter =
             this->option_table->begin();
         iter != this->option_table->end(); ++iter)
    {
        std::string const& arg = (*iter).first;
        OptionEntry& oe = (*iter).second;
        std::string base = "--" + arg;
        if (oe.param_arg_handler)
        {
            completions.insert(base + "=");
        }
        if (! oe.parameter_needed)
        {
            completions.insert(base);
        }
    }
}

void
ArgParser::handleCompletion()
{
    std::string extra_prefix;
    if (this->completions.empty())
    {
        // Detect --option=... Bash treats the = as a word separator.
        std::string choice_option;
        if (bash_cur.empty() && (bash_prev.length() > 2) &&
            (bash_prev.at(0) == '-') &&
            (bash_prev.at(1) == '-') &&
            (bash_line.at(bash_line.length() - 1) == '='))
        {
            choice_option = bash_prev.substr(2, std::string::npos);
        }
        else if ((bash_prev == "=") &&
                 (bash_line.length() > (bash_cur.length() + 1)))
        {
            // We're sitting at --option=x. Find previous option.
            size_t end_mark = bash_line.length() - bash_cur.length() - 1;
            char before_cur = bash_line.at(end_mark);
            if (before_cur == '=')
            {
                size_t space = bash_line.find_last_of(' ', end_mark);
                if (space != std::string::npos)
                {
                    std::string candidate =
                        bash_line.substr(space + 1, end_mark - space - 1);
                    if ((candidate.length() > 2) &&
                        (candidate.at(0) == '-') &&
                        (candidate.at(1) == '-'))
                    {
                        choice_option =
                            candidate.substr(2, std::string::npos);
                    }
                }
            }
        }
        if (! choice_option.empty())
        {
            if (zsh_completion)
            {
                // zsh wants --option=choice rather than just choice
                extra_prefix = "--" + choice_option + "=";
            }
            addChoicesToCompletions(choice_option, extra_prefix);
        }
        else if ((! bash_cur.empty()) && (bash_cur.at(0) == '-'))
        {
            addOptionsToCompletions();
            if (this->argc == 1)
            {
                // Help options are valid only by themselves.
                for (std::map<std::string, OptionEntry>::iterator iter =
                         this->help_option_table.begin();
                     iter != this->help_option_table.end(); ++iter)
                {
                    this->completions.insert("--" + (*iter).first);
                }
            }
        }
    }
    std::string prefix = extra_prefix + bash_cur;
    for (std::set<std::string>::iterator iter = completions.begin();
         iter != completions.end(); ++iter)
    {
        if (prefix.empty() ||
            ((*iter).substr(0, prefix.length()) == prefix))
        {
            std::cout << *iter << std::endl;
        }
    }
    exit(0);
}

static void set_qpdf_options(QPDF& pdf, Options& o)
{
    if (o.ignore_xref_streams)
    {
        pdf.setIgnoreXRefStreams(true);
    }
    if (o.suppress_recovery)
    {
        pdf.setAttemptRecovery(false);
    }
    if (o.password_is_hex_key)
    {
        pdf.setPasswordIsHexKey(true);
    }
    if (o.suppress_warnings)
    {
        pdf.setSuppressWarnings(true);
    }
}

static void do_check(QPDF& pdf, Options& o, int& exit_code)
{
    // Code below may set okay to false but not to true.
    // We assume okay until we prove otherwise but may
    // continue to perform additional checks after finding
    // errors.
    bool okay = true;
    std::cout << "checking " << o.infilename << std::endl;
    try
    {
        int extension_level = pdf.getExtensionLevel();
        std::cout << "PDF Version: " << pdf.getPDFVersion();
        if (extension_level > 0)
        {
            std::cout << " extension level "
                      << pdf.getExtensionLevel();
        }
        std::cout << std::endl;
        show_encryption(pdf, o);
        if (pdf.isLinearized())
        {
            std::cout << "File is linearized\n";
            if (! pdf.checkLinearization())
            {
                // any errors are reported by checkLinearization()
                okay = false;
            }
        }
        else
        {
            std::cout << "File is not linearized\n";
        }

        // Write the file no nowhere, uncompressing
        // streams.  This causes full file traversal and
        // decoding of all streams we can decode.
        QPDFWriter w(pdf);
        Pl_Discard discard;
        w.setOutputPipeline(&discard);
        w.setDecodeLevel(qpdf_dl_all);
        w.write();

        // Parse all content streams
        QPDFPageDocumentHelper dh(pdf);
        std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
        DiscardContents discard_contents;
        int pageno = 0;
        for (std::vector<QPDFPageObjectHelper>::iterator iter =
                 pages.begin();
             iter != pages.end(); ++iter)
        {
            QPDFPageObjectHelper& page(*iter);
            ++pageno;
            try
            {
                page.parsePageContents(&discard_contents);
            }
            catch (QPDFExc& e)
            {
                okay = false;
                std::cout << "page " << pageno << ": "
                          << e.what() << std::endl;
            }
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
        okay = false;
    }
    if (okay)
    {
        if (! pdf.getWarnings().empty())
        {
            exit_code = EXIT_WARNING;
        }
        else
        {
            std::cout << "No syntax or stream encoding errors"
                      << " found; the file may still contain"
                      << std::endl
                      << "errors that qpdf cannot detect"
                      << std::endl;
        }
    }
    else
    {
        exit_code = EXIT_ERROR;
    }
}

static void do_show_obj(QPDF& pdf, Options& o, int& exit_code)
{
    QPDFObjectHandle obj;
    if (o.show_trailer)
    {
        obj = pdf.getTrailer();
    }
    else
    {
        obj = pdf.getObjectByID(o.show_obj, o.show_gen);
    }
    if (obj.isStream())
    {
        if (o.show_raw_stream_data || o.show_filtered_stream_data)
        {
            bool filter = o.show_filtered_stream_data;
            if (filter &&
                (! obj.pipeStreamData(0, 0, qpdf_dl_all)))
            {
                QTC::TC("qpdf", "qpdf unable to filter");
                std::cerr << "Unable to filter stream data."
                          << std::endl;
                exit_code = EXIT_ERROR;
            }
            else
            {
                QUtil::binary_stdout();
                Pl_StdioFile out("stdout", stdout);
                obj.pipeStreamData(
                    &out,
                    (filter && o.normalize) ? qpdf_ef_normalize : 0,
                    filter ? qpdf_dl_all : qpdf_dl_none);
            }
        }
        else
        {
            std::cout
                << "Object is stream.  Dictionary:" << std::endl
                << obj.getDict().unparseResolved() << std::endl;
        }
    }
    else
    {
        std::cout << obj.unparseResolved() << std::endl;
    }
}

static void do_show_pages(QPDF& pdf, Options& o)
{
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
    int pageno = 0;
    for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
         iter != pages.end(); ++iter)
    {
        QPDFPageObjectHelper& ph(*iter);
        QPDFObjectHandle page = ph.getObjectHandle();
        ++pageno;

        std::cout << "page " << pageno << ": "
                  << page.getObjectID() << " "
                  << page.getGeneration() << " R" << std::endl;
        if (o.show_page_images)
        {
            std::map<std::string, QPDFObjectHandle> images =
                ph.getPageImages();
            if (! images.empty())
            {
                std::cout << "  images:" << std::endl;
                for (std::map<std::string,
                         QPDFObjectHandle>::iterator
                         iter = images.begin();
                     iter != images.end(); ++iter)
                {
                    std::string const& name = (*iter).first;
                    QPDFObjectHandle image = (*iter).second;
                    QPDFObjectHandle dict = image.getDict();
                    int width =
                        dict.getKey("/Width").getIntValue();
                    int height =
                        dict.getKey("/Height").getIntValue();
                    std::cout << "    " << name << ": "
                              << image.unparse()
                              << ", " << width << " x " << height
                              << std::endl;
                }
            }
        }

        std::cout << "  content:" << std::endl;
        std::vector<QPDFObjectHandle> content =
            ph.getPageContents();
        for (std::vector<QPDFObjectHandle>::iterator iter =
                 content.begin();
             iter != content.end(); ++iter)
        {
            std::cout << "    " << (*iter).unparse() << std::endl;
        }
    }
}

static void do_json_objects(QPDF& pdf, Options& o, JSON& j)
{
    // Add all objects. Do this first before other code below modifies
    // things by doing stuff like calling
    // pushInheritedAttributesToPage.
    bool all_objects = o.json_objects.empty();
    std::set<QPDFObjGen> wanted_og;
    for (std::set<std::string>::iterator iter = o.json_objects.begin();
         iter != o.json_objects.end(); ++iter)
    {
        bool trailer;
        int obj = 0;
        int gen = 0;
        parse_object_id(*iter, trailer, obj, gen);
        if (obj)
        {
            wanted_og.insert(QPDFObjGen(obj, gen));
        }
    }
    JSON j_objects = j.addDictionaryMember("objects", JSON::makeDictionary());
    if (all_objects || o.json_objects.count("trailer"))
    {
        j_objects.addDictionaryMember(
            "trailer", pdf.getTrailer().getJSON(true));
    }
    std::vector<QPDFObjectHandle> objects = pdf.getAllObjects();
    for (std::vector<QPDFObjectHandle>::iterator iter = objects.begin();
         iter != objects.end(); ++iter)
    {
        if (all_objects || wanted_og.count((*iter).getObjGen()))
        {
            j_objects.addDictionaryMember(
                (*iter).unparse(), (*iter).getJSON(true));
        }
    }
}

static void do_json_pages(QPDF& pdf, Options& o, JSON& j)
{
    JSON j_pages = j.addDictionaryMember("pages", JSON::makeArray());
    QPDFPageDocumentHelper pdh(pdf);
    QPDFPageLabelDocumentHelper pldh(pdf);
    QPDFOutlineDocumentHelper odh(pdf);
    pdh.pushInheritedAttributesToPage();
    std::vector<QPDFPageObjectHelper> pages = pdh.getAllPages();
    size_t pageno = 0;
    for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
         iter != pages.end(); ++iter, ++pageno)
    {
        JSON j_page = j_pages.addArrayElement(JSON::makeDictionary());
        QPDFPageObjectHelper& ph(*iter);
        QPDFObjectHandle page = ph.getObjectHandle();
        j_page.addDictionaryMember("object", page.getJSON());
        JSON j_images = j_page.addDictionaryMember(
            "images", JSON::makeArray());
        std::map<std::string, QPDFObjectHandle> images =
            ph.getPageImages();
        for (std::map<std::string, QPDFObjectHandle>::iterator iter =
                 images.begin();
             iter != images.end(); ++iter)
        {
            JSON j_image = j_images.addArrayElement(JSON::makeDictionary());
            j_image.addDictionaryMember(
                "name", JSON::makeString((*iter).first));
            QPDFObjectHandle image = (*iter).second;
            QPDFObjectHandle dict = image.getDict();
            j_image.addDictionaryMember("object", image.getJSON());
            j_image.addDictionaryMember(
                "width", dict.getKey("/Width").getJSON());
            j_image.addDictionaryMember(
                "height", dict.getKey("/Height").getJSON());
            j_image.addDictionaryMember(
                "colorspace", dict.getKey("/ColorSpace").getJSON());
            j_image.addDictionaryMember(
                "bitspercomponent", dict.getKey("/BitsPerComponent").getJSON());
            QPDFObjectHandle filters = dict.getKey("/Filter").wrapInArray();
            j_image.addDictionaryMember(
                "filter", filters.getJSON());
            QPDFObjectHandle decode_parms = dict.getKey("/DecodeParms");
            QPDFObjectHandle dp_array;
            if (decode_parms.isArray())
            {
                dp_array = decode_parms;
            }
            else
            {
                dp_array = QPDFObjectHandle::newArray();
                for (int i = 0; i < filters.getArrayNItems(); ++i)
                {
                    dp_array.appendItem(decode_parms);
                }
            }
            j_image.addDictionaryMember("decodeparms", dp_array.getJSON());
            j_image.addDictionaryMember(
                "filterable",
                JSON::makeBool(
                    image.pipeStreamData(0, 0, o.decode_level, true)));
        }
        j_page.addDictionaryMember("images", j_images);
        JSON j_contents = j_page.addDictionaryMember(
            "contents", JSON::makeArray());
        std::vector<QPDFObjectHandle> content = ph.getPageContents();
        for (std::vector<QPDFObjectHandle>::iterator iter = content.begin();
             iter != content.end(); ++iter)
        {
            j_contents.addArrayElement((*iter).getJSON());
        }
        j_page.addDictionaryMember(
            "label", pldh.getLabelForPage(pageno).getJSON());
        JSON j_outlines = j_page.addDictionaryMember(
            "outlines", JSON::makeArray());
        std::list<QPDFOutlineObjectHelper> outlines =
            odh.getOutlinesForPage(page.getObjGen());
        for (std::list<QPDFOutlineObjectHelper>::iterator oiter =
                 outlines.begin();
             oiter != outlines.end(); ++oiter)
        {
            JSON j_outline = j_outlines.addArrayElement(JSON::makeDictionary());
            j_outline.addDictionaryMember(
                "object", (*oiter).getObjectHandle().getJSON());
            j_outline.addDictionaryMember(
                "title", JSON::makeString((*oiter).getTitle()));
            j_outline.addDictionaryMember(
                "dest", (*oiter).getDest().getJSON(true));
        }
        j_page.addDictionaryMember("pageposfrom1", JSON::makeInt(1 + pageno));
    }
}

static void do_json_page_labels(QPDF& pdf, Options& o, JSON& j)
{
    JSON j_labels = j.addDictionaryMember("pagelabels", JSON::makeArray());
    QPDFPageLabelDocumentHelper pldh(pdf);
    QPDFPageDocumentHelper pdh(pdf);
    std::vector<QPDFPageObjectHelper> pages = pdh.getAllPages();
    if (pldh.hasPageLabels())
    {
        std::vector<QPDFObjectHandle> labels;
        pldh.getLabelsForPageRange(0, pages.size() - 1, 0, labels);
        for (std::vector<QPDFObjectHandle>::iterator iter = labels.begin();
             iter != labels.end(); ++iter)
        {
            std::vector<QPDFObjectHandle>::iterator next = iter;
            ++next;
            if (next == labels.end())
            {
                // This can't happen, so ignore it. This could only
                // happen if getLabelsForPageRange somehow returned an
                // odd number of items.
                break;
            }
            JSON j_label = j_labels.addArrayElement(JSON::makeDictionary());
            j_label.addDictionaryMember("index", (*iter).getJSON());
            ++iter;
            j_label.addDictionaryMember("label", (*iter).getJSON());
        }
    }
}

static void add_outlines_to_json(
    std::list<QPDFOutlineObjectHelper> outlines, JSON& j,
    std::map<QPDFObjGen, int>& page_numbers)
{
    for (std::list<QPDFOutlineObjectHelper>::iterator iter = outlines.begin();
         iter != outlines.end(); ++iter)
    {
        QPDFOutlineObjectHelper& ol = *iter;
        JSON jo = j.addArrayElement(JSON::makeDictionary());
        jo.addDictionaryMember("object", ol.getObjectHandle().getJSON());
        jo.addDictionaryMember("title", JSON::makeString(ol.getTitle()));
        jo.addDictionaryMember("dest", ol.getDest().getJSON(true));
        jo.addDictionaryMember("open", JSON::makeBool(ol.getCount() >= 0));
        QPDFObjectHandle page = ol.getDestPage();
        JSON j_destpage = JSON::makeNull();
        if (page.isIndirect())
        {
            QPDFObjGen og = page.getObjGen();
            if (page_numbers.count(og))
            {
                j_destpage = JSON::makeInt(page_numbers[og]);
            }
        }
        jo.addDictionaryMember("destpageposfrom1", j_destpage);
        JSON j_kids = jo.addDictionaryMember("kids", JSON::makeArray());
        add_outlines_to_json(ol.getKids(), j_kids, page_numbers);
    }
}

static void do_json_outlines(QPDF& pdf, Options& o, JSON& j)
{
    std::map<QPDFObjGen, int> page_numbers;
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
    int n = 0;
    for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
	 iter != pages.end(); ++iter)
    {
	QPDFObjectHandle oh = (*iter).getObjectHandle();
	page_numbers[oh.getObjGen()] = ++n;
    }

    JSON j_outlines = j.addDictionaryMember(
        "outlines", JSON::makeArray());
    QPDFOutlineDocumentHelper odh(pdf);
    add_outlines_to_json(odh.getTopLevelOutlines(), j_outlines, page_numbers);
}

static void do_json_acroform(QPDF& pdf, Options& o, JSON& j)
{
    JSON j_acroform = j.addDictionaryMember(
        "acroform", JSON::makeDictionary());
    QPDFAcroFormDocumentHelper afdh(pdf);
    j_acroform.addDictionaryMember(
        "hasacroform",
        JSON::makeBool(afdh.hasAcroForm()));
    j_acroform.addDictionaryMember(
        "needappearances",
        JSON::makeBool(afdh.getNeedAppearances()));
    JSON j_fields = j_acroform.addDictionaryMember(
        "fields", JSON::makeArray());
    QPDFPageDocumentHelper pdh(pdf);
    std::vector<QPDFPageObjectHelper> pages = pdh.getAllPages();
    int pagepos1 = 0;
    for (std::vector<QPDFPageObjectHelper>::iterator page_iter =
             pages.begin();
         page_iter != pages.end(); ++page_iter)
    {
        ++pagepos1;
        std::vector<QPDFAnnotationObjectHelper> annotations =
            afdh.getWidgetAnnotationsForPage(*page_iter);
        for (std::vector<QPDFAnnotationObjectHelper>::iterator annot_iter =
                 annotations.begin();
             annot_iter != annotations.end(); ++annot_iter)
        {
            QPDFAnnotationObjectHelper& aoh = *annot_iter;
            QPDFFormFieldObjectHelper ffh =
                afdh.getFieldForAnnotation(aoh);
            JSON j_field = j_fields.addArrayElement(
                JSON::makeDictionary());
            j_field.addDictionaryMember(
                "object",
                ffh.getObjectHandle().getJSON());
            j_field.addDictionaryMember(
                "parent",
                ffh.getObjectHandle().getKey("/Parent").getJSON());
            j_field.addDictionaryMember(
                "pageposfrom1",
                JSON::makeInt(pagepos1));
            j_field.addDictionaryMember(
                "fieldtype",
                JSON::makeString(ffh.getFieldType()));
            j_field.addDictionaryMember(
                "fieldflags",
                JSON::makeInt(ffh.getFlags()));
            j_field.addDictionaryMember(
                "fullname",
                JSON::makeString(ffh.getFullyQualifiedName()));
            j_field.addDictionaryMember(
                "partialname",
                JSON::makeString(ffh.getPartialName()));
            j_field.addDictionaryMember(
                "alternativename",
                JSON::makeString(ffh.getAlternativeName()));
            j_field.addDictionaryMember(
                "mappingname",
                JSON::makeString(ffh.getMappingName()));
            j_field.addDictionaryMember(
                "value",
                ffh.getValue().getJSON());
            j_field.addDictionaryMember(
                "defaultvalue",
                ffh.getDefaultValue().getJSON());
            j_field.addDictionaryMember(
                "quadding",
                JSON::makeInt(ffh.getQuadding()));
            j_field.addDictionaryMember(
                "ischeckbox",
                JSON::makeBool(ffh.isCheckbox()));
            j_field.addDictionaryMember(
                "isradiobutton",
                JSON::makeBool(ffh.isRadioButton()));
            j_field.addDictionaryMember(
                "ischoice",
                JSON::makeBool(ffh.isChoice()));
            j_field.addDictionaryMember(
                "istext",
                JSON::makeBool(ffh.isText()));
            JSON j_choices = j_field.addDictionaryMember(
                "choices", JSON::makeArray());
            std::vector<std::string> choices = ffh.getChoices();
            for (std::vector<std::string>::iterator iter = choices.begin();
                 iter != choices.end(); ++iter)
            {
                j_choices.addArrayElement(JSON::makeString(*iter));
            }
            JSON j_annot = j_field.addDictionaryMember(
                "annotation", JSON::makeDictionary());
            j_annot.addDictionaryMember(
                "object",
                aoh.getObjectHandle().getJSON());
            j_annot.addDictionaryMember(
                "appearancestate",
                JSON::makeString(aoh.getAppearanceState()));
            j_annot.addDictionaryMember(
                "annotationflags",
                JSON::makeInt(aoh.getFlags()));
        }
    }
}

static void do_json(QPDF& pdf, Options& o)
{
    JSON j = JSON::makeDictionary();
    // This version is updated every time a non-backward-compatible
    // change is made to the JSON format. Clients of the JSON are to
    // ignore unrecognized keys, so we only update the version of a
    // key disappears or if its value changes meaning.
    j.addDictionaryMember("version", JSON::makeInt(1));
    JSON j_params = j.addDictionaryMember(
        "parameters", JSON::makeDictionary());
    std::string decode_level_str;
    switch (o.decode_level)
    {
        case qpdf_dl_none:
          decode_level_str = "none";
          break;
        case qpdf_dl_generalized:
          decode_level_str = "generalized";
          break;
        case qpdf_dl_specialized:
          decode_level_str = "specialized";
          break;
        case qpdf_dl_all:
          decode_level_str = "all";
          break;
    }
    j_params.addDictionaryMember(
        "decodelevel", JSON::makeString(decode_level_str));

    bool all_keys = o.json_keys.empty();
    // The list of selectable top-level keys id duplicated in three
    // places: json_schema, do_json, and initOptionTable.
    if (all_keys || o.json_keys.count("objects"))
    {
        do_json_objects(pdf, o, j);
    }
    if (all_keys || o.json_keys.count("pages"))
    {
        do_json_pages(pdf, o, j);
    }
    if (all_keys || o.json_keys.count("pagelabels"))
    {
        do_json_page_labels(pdf, o, j);
    }
    if (all_keys || o.json_keys.count("outlines"))
    {
        do_json_outlines(pdf, o, j);
    }
    if (all_keys || o.json_keys.count("acroform"))
    {
        do_json_acroform(pdf, o, j);
    }

    // Check against schema

    JSON schema = json_schema(&o.json_keys);
    std::list<std::string> errors;
    if (! j.checkSchema(schema, errors))
    {
        std::cerr
            << whoami << " didn't create JSON that complies with its own\n\
rules. Please report this as a bug at\n\
   https://github.com/qpdf/qpdf/issues/new\n\
ideally with the file that caused the error and the output below. Thanks!\n\
\n";
        for (std::list<std::string>::iterator iter = errors.begin();
             iter != errors.end(); ++iter)
        {
            std::cerr << (*iter) << std::endl;
        }
    }

    std::cout << j.unparse() << std::endl;
}

static void do_inspection(QPDF& pdf, Options& o)
{
    int exit_code = 0;
    if (o.check)
    {
        do_check(pdf, o, exit_code);
    }
    if (o.json)
    {
        do_json(pdf, o);
    }
    if (o.show_npages)
    {
        QTC::TC("qpdf", "qpdf npages");
        std::cout << pdf.getRoot().getKey("/Pages").
            getKey("/Count").getIntValue() << std::endl;
    }
    if (o.show_encryption)
    {
        show_encryption(pdf, o);
    }
    if (o.check_linearization)
    {
        if (pdf.checkLinearization())
        {
            std::cout << o.infilename << ": no linearization errors"
                      << std::endl;
        }
        else
        {
            exit_code = EXIT_ERROR;
        }
    }
    if (o.show_linearization)
    {
        if (pdf.isLinearized())
        {
            pdf.showLinearizationData();
        }
        else
        {
            std::cout << o.infilename << " is not linearized"
                      << std::endl;
        }
    }
    if (o.show_xref)
    {
        pdf.showXRefTable();
    }
    if ((o.show_obj > 0) || o.show_trailer)
    {
        do_show_obj(pdf, o, exit_code);
    }
    if (o.show_pages)
    {
        do_show_pages(pdf, o);
    }
    if (exit_code)
    {
        exit(exit_code);
    }
}

class ImageOptimizer: public QPDFObjectHandle::StreamDataProvider
{
  public:
    ImageOptimizer(Options& o, QPDFObjectHandle& image);
    virtual ~ImageOptimizer()
    {
    }
    virtual void provideStreamData(int objid, int generation,
				   Pipeline* pipeline);
    PointerHolder<Pipeline> makePipeline(
        std::string const& description, Pipeline* next);
    bool evaluate(std::string const& description);

  private:
    Options& o;
    QPDFObjectHandle image;
};

ImageOptimizer::ImageOptimizer(Options& o, QPDFObjectHandle& image) :
    o(o),
    image(image)
{
}

PointerHolder<Pipeline>
ImageOptimizer::makePipeline(std::string const& description, Pipeline* next)
{
    PointerHolder<Pipeline> result;
    QPDFObjectHandle dict = image.getDict();
    QPDFObjectHandle w_obj = dict.getKey("/Width");
    QPDFObjectHandle h_obj = dict.getKey("/Height");
    QPDFObjectHandle colorspace_obj = dict.getKey("/ColorSpace");
    if (! (w_obj.isNumber() && h_obj.isNumber()))
    {
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because image dictionary"
                      << " is missing required keys" << std::endl;
        }
        return result;
    }
    QPDFObjectHandle components_obj = dict.getKey("/BitsPerComponent");
    if (! (components_obj.isInteger() && (components_obj.getIntValue() == 8)))
    {
        QTC::TC("qpdf", "qpdf image optimize bits per component");
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because image has other than"
                      << " 8 bits per component" << std::endl;
        }
        return result;
    }
    // Files have been seen in the wild whose width and height are
    // floating point, which is goofy, but we can deal with it.
    JDIMENSION w = static_cast<JDIMENSION>(
        w_obj.isInteger() ? w_obj.getIntValue() : w_obj.getNumericValue());
    JDIMENSION h = static_cast<JDIMENSION>(
        h_obj.isInteger() ? h_obj.getIntValue() : h_obj.getNumericValue());
    std::string colorspace = (colorspace_obj.isName() ?
                              colorspace_obj.getName() :
                              std::string());
    int components = 0;
    J_COLOR_SPACE cs = JCS_UNKNOWN;
    if (colorspace == "/DeviceRGB")
    {
        components = 3;
        cs = JCS_RGB;
    }
    else if (colorspace == "/DeviceGray")
    {
        components = 1;
        cs = JCS_GRAYSCALE;
    }
    else if (colorspace == "/DeviceCMYK")
    {
        components = 4;
        cs = JCS_CMYK;
    }
    else
    {
        QTC::TC("qpdf", "qpdf image optimize colorspace");
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because qpdf can't optimize"
                      << " images with this colorspace" << std::endl;
        }
        return result;
    }
    if (((o.oi_min_width > 0) && (w <= o.oi_min_width)) ||
        ((o.oi_min_height > 0) && (h <= o.oi_min_height)) ||
        ((o.oi_min_area > 0) && ((w * h) <= o.oi_min_area)))
    {
        QTC::TC("qpdf", "qpdf image optimize too small");
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because image"
                      << " is smaller than requested minimum dimensions"
                      << std::endl;
        }
        return result;
    }

    result = new Pl_DCT("jpg", next, w, h, components, cs);
    return result;
}

bool
ImageOptimizer::evaluate(std::string const& description)
{
    if (! image.pipeStreamData(0, 0, qpdf_dl_specialized, true))
    {
        QTC::TC("qpdf", "qpdf image optimize no pipeline");
        if (o.verbose)
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because unable to decode data"
                      << " or data already uses DCT"
                      << std::endl;
        }
        return false;
    }
    Pl_Discard d;
    Pl_Count c("count", &d);
    PointerHolder<Pipeline> p = makePipeline(description, &c);
    if (p.getPointer() == 0)
    {
        // message issued by makePipeline
        return false;
    }
    if (! image.pipeStreamData(p.getPointer(), 0, qpdf_dl_specialized))
    {
        return false;
    }
    long long orig_length = image.getDict().getKey("/Length").getIntValue();
    if (c.getCount() >= orig_length)
    {
        QTC::TC("qpdf", "qpdf image optimize no shrink");
        if (o.verbose)
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because DCT compression does not"
                      << " reduce image size" << std::endl;
        }
        return false;
    }
    if (o.verbose)
    {
        std::cout << whoami << ": " << description
                  << ": optimizing image reduces size from "
                  << orig_length << " to " << c.getCount()
                  << std::endl;
    }
    return true;
}

void
ImageOptimizer::provideStreamData(int, int, Pipeline* pipeline)
{
    PointerHolder<Pipeline> p = makePipeline("", pipeline);
    if (p.getPointer() == 0)
    {
        // Should not be possible
        image.warnIfPossible("unable to create pipeline after previous"
                             " success; image data will be lost");
        pipeline->finish();
        return;
    }
    image.pipeStreamData(p.getPointer(), 0, qpdf_dl_specialized,
                         false, false);
}

template <typename T>
static PointerHolder<QPDF> do_process_once(
    void (QPDF::*fn)(T, char const*),
    T item, char const* password,
    Options& o, bool empty)
{
    PointerHolder<QPDF> pdf = new QPDF;
    set_qpdf_options(*pdf, o);
    if (empty)
    {
        pdf->emptyPDF();
    }
    else
    {
        ((*pdf).*fn)(item, password);
    }
    return pdf;
}

template <typename T>
static PointerHolder<QPDF> do_process(
    void (QPDF::*fn)(T, char const*),
    T item, char const* password,
    Options& o, bool empty)
{
    // If a password has been specified but doesn't work, try other
    // passwords that are equivalent in different character encodings.
    // This makes it possible to open PDF files that were encrypted
    // using incorrect string encodings. For example, if someone used
    // a password encoded in PDF Doc encoding or Windows code page
    // 1252 for an AES-encrypted file or a UTF-8-encoded password on
    // an RC4-encrypted file, or if the password was properly encoded
    // by the password given here was incorrectly encoded, there's a
    // good chance we'd succeed here.

    std::string ptemp;
    if (password && (! o.password_is_hex_key))
    {
        if (o.password_mode == pm_hex_bytes)
        {
            // Special case: handle --password-mode=hex-bytes for input
            // password as well as output password
            QTC::TC("qpdf", "qpdf input password hex-bytes");
            ptemp = QUtil::hex_decode(password);
            password = ptemp.c_str();
        }
    }
    if ((password == 0) || empty || o.password_is_hex_key ||
        o.suppress_password_recovery)
    {
        // There is no password, or we're not doing recovery, so just
        // do the normal processing with the supplied password.
        return do_process_once(fn, item, password, o, empty);
    }

    // Get a list of otherwise encoded strings. Keep in scope for this
    // method.
    std::vector<std::string> passwords_str =
        QUtil::possible_repaired_encodings(password);
    // Represent to char const*, as required by the QPDF class.
    std::vector<char const*> passwords;
    for (std::vector<std::string>::iterator iter = passwords_str.begin();
         iter != passwords_str.end(); ++iter)
    {
        passwords.push_back((*iter).c_str());
    }
    // We always try the supplied password first because it is the
    // first string returned by possible_repaired_encodings. If there
    // is more than one option, go ahead and put the supplied password
    // at the end so that it's that decoding attempt whose exception
    // is thrown.
    if (passwords.size() > 1)
    {
        passwords.push_back(password);
    }

    // Try each password. If one works, return the resulting object.
    // If they all fail, throw the exception thrown by the final
    // attempt, which, like the first attempt, will be with the
    // supplied password.
    bool warned = false;
    for (std::vector<char const*>::iterator iter = passwords.begin();
         iter != passwords.end(); ++iter)
    {
        try
        {
            return do_process_once(fn, item, *iter, o, empty);
        }
        catch (QPDFExc& e)
        {
            std::vector<char const*>::iterator next = iter;
            ++next;
            if (next == passwords.end())
            {
                throw e;
            }
        }
        if ((! warned) && o.verbose)
        {
            warned = true;
            std::cout << whoami << ": supplied password didn't work;"
                      << " trying other passwords based on interpreting"
                      << " password with different string encodings"
                      << std::endl;
        }
    }
    // Should not be reachable
    throw std::logic_error("do_process returned");
}

static PointerHolder<QPDF> process_file(char const* filename,
                                        char const* password,
                                        Options& o)
{
    return do_process(&QPDF::processFile, filename, password, o,
                      strcmp(filename, "") == 0);
}

static PointerHolder<QPDF> process_input_source(
    PointerHolder<InputSource> is, char const* password, Options& o)
{
    return do_process(&QPDF::processInputSource, is, password, o, false);
}

static void validate_under_overlay(QPDF& pdf, UnderOverlay* uo, Options& o)
{
    if (0 == uo->filename)
    {
        return;
    }
    QPDFPageDocumentHelper main_pdh(pdf);
    int main_npages = static_cast<int>(main_pdh.getAllPages().size());
    uo->pdf = process_file(uo->filename, uo->password, o);
    QPDFPageDocumentHelper uo_pdh(*(uo->pdf));
    int uo_npages = static_cast<int>(uo_pdh.getAllPages().size());
    try
    {
        uo->to_pagenos = QUtil::parse_numrange(uo->to_nr, main_npages);
    }
    catch (std::runtime_error& e)
    {
        usageExit("parsing numeric range for " + uo->which +
                  " \"to\" pages: " + e.what());
    }
    try
    {
        if (0 == strlen(uo->from_nr))
        {
            QTC::TC("qpdf", "qpdf from_nr from repeat_nr");
            uo->from_nr = uo->repeat_nr;
        }
        uo->from_pagenos = QUtil::parse_numrange(uo->from_nr, uo_npages);
        if (strlen(uo->repeat_nr))
        {
            uo->repeat_pagenos =
                QUtil::parse_numrange(uo->repeat_nr, uo_npages);
        }
    }
    catch (std::runtime_error& e)
    {
        usageExit("parsing numeric range for " + uo->which + " file " +
                  uo->filename + ": " + e.what());
    }
}

static void get_uo_pagenos(UnderOverlay& uo,
                           std::map<int, std::vector<int> >& pagenos)
{
    size_t idx = 0;
    size_t from_size = uo.from_pagenos.size();
    size_t repeat_size = uo.repeat_pagenos.size();
    for (std::vector<int>::iterator iter = uo.to_pagenos.begin();
         iter != uo.to_pagenos.end(); ++iter, ++idx)
    {
        if (idx < from_size)
        {
            pagenos[*iter].push_back(uo.from_pagenos.at(idx));
        }
        else if (repeat_size)
        {
            pagenos[*iter].push_back(
                uo.repeat_pagenos.at((idx - from_size) % repeat_size));
        }
    }
}

static void do_under_overlay_for_page(
    QPDF& pdf,
    Options& o,
    UnderOverlay& uo,
    std::map<int, std::vector<int> >& pagenos,
    size_t page_idx,
    std::map<int, QPDFObjectHandle>& fo,
    std::vector<QPDFPageObjectHelper>& pages,
    QPDFPageObjectHelper& dest_page,
    bool before)
{
    int pageno = 1 + page_idx;
    if (! pagenos.count(pageno))
    {
        return;
    }

    std::string content;
    int min_suffix = 1;
    QPDFObjectHandle resources = dest_page.getAttribute("/Resources", true);
    for (std::vector<int>::iterator iter = pagenos[pageno].begin();
         iter != pagenos[pageno].end(); ++iter)
    {
        int from_pageno = *iter;
        if (o.verbose)
        {
            std::cout << "    " << uo.which << " " << from_pageno << std::endl;
        }
        if (0 == fo.count(from_pageno))
        {
            fo[from_pageno] =
                pdf.copyForeignObject(
                    pages.at(from_pageno - 1).getFormXObjectForPage());
        }
        // If the same page is overlaid or underlaid multiple times,
        // we'll generate multiple names for it, but that's harmless
        // and also a pretty goofy case that's not worth coding
        // around.
        std::string name = resources.getUniqueResourceName("/Fx", min_suffix);
        std::string new_content = dest_page.placeFormXObject(
            fo[from_pageno], name,
            dest_page.getTrimBox().getArrayAsRectangle());
        if (! new_content.empty())
        {
            resources.mergeResources(
                QPDFObjectHandle::parse("<< /XObject << >> >>"));
            resources.getKey("/XObject").replaceKey(name, fo[from_pageno]);
            ++min_suffix;
            content += new_content;
        }
    }
    if (! content.empty())
    {
        if (before)
        {
            dest_page.addPageContents(
                QPDFObjectHandle::newStream(&pdf, content), true);
        }
        else
        {
            dest_page.addPageContents(
                QPDFObjectHandle::newStream(&pdf, "q\n"), true);
            dest_page.addPageContents(
                QPDFObjectHandle::newStream(&pdf, "\nQ\n" + content), false);
        }
    }
}

static void handle_under_overlay(QPDF& pdf, Options& o)
{
    validate_under_overlay(pdf, &o.underlay, o);
    validate_under_overlay(pdf, &o.overlay, o);
    if ((0 == o.underlay.pdf.getPointer()) &&
        (0 == o.overlay.pdf.getPointer()))
    {
        return;
    }
    std::map<int, std::vector<int> > underlay_pagenos;
    get_uo_pagenos(o.underlay, underlay_pagenos);
    std::map<int, std::vector<int> > overlay_pagenos;
    get_uo_pagenos(o.overlay, overlay_pagenos);
    std::map<int, QPDFObjectHandle> underlay_fo;
    std::map<int, QPDFObjectHandle> overlay_fo;
    std::vector<QPDFPageObjectHelper> upages;
    if (o.underlay.pdf.getPointer())
    {
        upages = QPDFPageDocumentHelper(*(o.underlay.pdf)).getAllPages();
    }
    std::vector<QPDFPageObjectHelper> opages;
    if (o.overlay.pdf.getPointer())
    {
        opages = QPDFPageDocumentHelper(*(o.overlay.pdf)).getAllPages();
    }

    QPDFPageDocumentHelper main_pdh(pdf);
    std::vector<QPDFPageObjectHelper> main_pages = main_pdh.getAllPages();
    size_t main_npages = main_pages.size();
    if (o.verbose)
    {
        std::cout << whoami << ": processing underlay/overlay" << std::endl;
    }
    for (size_t i = 0; i < main_npages; ++i)
    {
        if (o.verbose)
        {
            std::cout << "  page " << 1+i << std::endl;
        }
        do_under_overlay_for_page(pdf, o, o.underlay, underlay_pagenos, i,
                                  underlay_fo, upages, main_pages.at(i),
                                  true);
        do_under_overlay_for_page(pdf, o, o.overlay, overlay_pagenos, i,
                                  overlay_fo, opages, main_pages.at(i),
                                  false);
    }
}

static void handle_transformations(QPDF& pdf, Options& o)
{
    QPDFPageDocumentHelper dh(pdf);
    if (o.externalize_inline_images)
    {
        std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
        for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
             iter != pages.end(); ++iter)
        {
            QPDFPageObjectHelper& ph(*iter);
            ph.externalizeInlineImages(o.ii_min_bytes);
        }
    }
    if (o.optimize_images)
    {
        int pageno = 0;
        std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
        for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
             iter != pages.end(); ++iter)
        {
            ++pageno;
            QPDFPageObjectHelper& ph(*iter);
            QPDFObjectHandle page = ph.getObjectHandle();
            std::map<std::string, QPDFObjectHandle> images =
                ph.getPageImages();
            for (std::map<std::string, QPDFObjectHandle>::iterator iter =
                     images.begin();
                 iter != images.end(); ++iter)
            {
                std::string name = (*iter).first;
                QPDFObjectHandle& image = (*iter).second;
                ImageOptimizer* io = new ImageOptimizer(o, image);
                PointerHolder<QPDFObjectHandle::StreamDataProvider> sdp(io);
                if (io->evaluate("image " + name + " on page " +
                                 QUtil::int_to_string(pageno)))
                {
                    QPDFObjectHandle new_image =
                        QPDFObjectHandle::newStream(&pdf);
                    new_image.replaceDict(image.getDict().shallowCopy());
                    new_image.replaceStreamData(
                        sdp,
                        QPDFObjectHandle::newName("/DCTDecode"),
                        QPDFObjectHandle::newNull());
                    ph.getAttribute("/Resources", true).
                        getKey("/XObject").replaceKey(
                            name, new_image);
                }
            }
        }
    }
    if (o.generate_appearances)
    {
        QPDFAcroFormDocumentHelper afdh(pdf);
        afdh.generateAppearancesIfNeeded();
    }
    if (o.flatten_annotations)
    {
        dh.flattenAnnotations(o.flatten_annotations_required,
                              o.flatten_annotations_forbidden);
    }
    if (o.coalesce_contents)
    {
        std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
        for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
             iter != pages.end(); ++iter)
        {
            (*iter).coalesceContentStreams();
        }
    }
}

static void handle_page_specs(QPDF& pdf, Options& o)
{
    // Parse all page specifications and translate them into lists of
    // actual pages.

    // Handle "." as a shortcut for the input file
    for (std::vector<PageSpec>::iterator iter = o.page_specs.begin();
         iter != o.page_specs.end(); ++iter)
    {
        PageSpec& page_spec = *iter;
        if (page_spec.filename == ".")
        {
            page_spec.filename = o.infilename;
        }
    }

    if (! o.keep_files_open_set)
    {
        // Count the number of distinct files to determine whether we
        // should keep files open or not. Rather than trying to code
        // some portable heuristic based on OS limits, just hard-code
        // this at a given number and allow users to override.
        std::set<std::string> filenames;
        for (std::vector<PageSpec>::iterator iter = o.page_specs.begin();
             iter != o.page_specs.end(); ++iter)
        {
            PageSpec& page_spec = *iter;
            filenames.insert(page_spec.filename);
        }
        // NOTE: The number 200 for this threshold is in the help
        // message and manual and is baked into the test suite.
        if (filenames.size() > 200)
        {
            QTC::TC("qpdf", "qpdf disable keep files open");
            if (o.verbose)
            {
                std::cout << whoami << ": selecting --keep-open-files=n"
                          << std::endl;
            }
            o.keep_files_open = false;
        }
        else
        {
            if (o.verbose)
            {
                std::cout << whoami << ": selecting --keep-open-files=y"
                          << std::endl;
            }
            o.keep_files_open = true;
            QTC::TC("qpdf", "qpdf don't disable keep files open");
        }
    }

    // Create a QPDF object for each file that we may take pages from.
    std::vector<PointerHolder<QPDF> > page_heap;
    std::map<std::string, QPDF*> page_spec_qpdfs;
    std::map<std::string, ClosedFileInputSource*> page_spec_cfis;
    page_spec_qpdfs[o.infilename] = &pdf;
    std::vector<QPDFPageData> parsed_specs;
    std::map<unsigned long long, std::set<QPDFObjGen> > copied_pages;
    for (std::vector<PageSpec>::iterator iter = o.page_specs.begin();
         iter != o.page_specs.end(); ++iter)
    {
        PageSpec& page_spec = *iter;
        if (page_spec_qpdfs.count(page_spec.filename) == 0)
        {
            // Open the PDF file and store the QPDF object. Throw a
            // PointerHolder to the qpdf into a heap so that it
            // survives through copying to the output but gets cleaned up
            // automatically at the end. Do not canonicalize the file
            // name. Using two different paths to refer to the same
            // file is a document workaround for duplicating a page.
            // If you are using this an example of how to do this with
            // the API, you can just create two different QPDF objects
            // to the same underlying file with the same path to
            // achieve the same affect.
            char const* password = page_spec.password;
            if (o.encryption_file && (password == 0) &&
                (page_spec.filename == o.encryption_file))
            {
                QTC::TC("qpdf", "qpdf pages encryption password");
                password = o.encryption_file_password;
            }
            if (o.verbose)
            {
                std::cout << whoami << ": processing "
                          << page_spec.filename << std::endl;
            }
            PointerHolder<InputSource> is;
            ClosedFileInputSource* cis = 0;
            if (! o.keep_files_open)
            {
                QTC::TC("qpdf", "qpdf keep files open n");
                cis = new ClosedFileInputSource(page_spec.filename.c_str());
                is = cis;
                cis->stayOpen(true);
            }
            else
            {
                QTC::TC("qpdf", "qpdf keep files open y");
                FileInputSource* fis = new FileInputSource();
                is = fis;
                fis->setFilename(page_spec.filename.c_str());
            }
            PointerHolder<QPDF> qpdf_ph = process_input_source(is, password, o);
            page_heap.push_back(qpdf_ph);
            page_spec_qpdfs[page_spec.filename] = qpdf_ph.getPointer();
            if (cis)
            {
                cis->stayOpen(false);
                page_spec_cfis[page_spec.filename] = cis;
            }
        }

        // Read original pages from the PDF, and parse the page range
        // associated with this occurrence of the file.
        parsed_specs.push_back(
            QPDFPageData(page_spec.filename,
                         page_spec_qpdfs[page_spec.filename],
                         page_spec.range));
    }

    if (! o.preserve_unreferenced_page_resources)
    {
        for (std::map<std::string, QPDF*>::iterator iter =
                 page_spec_qpdfs.begin();
             iter != page_spec_qpdfs.end(); ++iter)
        {
            std::string const& filename = (*iter).first;
            ClosedFileInputSource* cis = 0;
            if (page_spec_cfis.count(filename))
            {
                cis = page_spec_cfis[filename];
                cis->stayOpen(true);
            }
            QPDFPageDocumentHelper dh(*((*iter).second));
            dh.removeUnreferencedResources();
            if (cis)
            {
                cis->stayOpen(false);
            }
        }
    }

    // Clear all pages out of the primary QPDF's pages tree but leave
    // the objects in place in the file so they can be re-added
    // without changing their object numbers. This enables other
    // things in the original file, such as outlines, to continue to
    // work.
    if (o.verbose)
    {
        std::cout << whoami
                  << ": removing unreferenced pages from primary input"
                  << std::endl;
    }
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> orig_pages = dh.getAllPages();
    for (std::vector<QPDFPageObjectHelper>::iterator iter =
             orig_pages.begin();
         iter != orig_pages.end(); ++iter)
    {
        dh.removePage(*iter);
    }

    if (o.collate && (parsed_specs.size() > 1))
    {
        // Collate the pages by selecting one page from each spec in
        // order. When a spec runs out of pages, stop selecting from
        // it.
        std::vector<QPDFPageData> new_parsed_specs;
        size_t nspecs = parsed_specs.size();
        size_t cur_page = 0;
        bool got_pages = true;
        while (got_pages)
        {
            got_pages = false;
            for (size_t i = 0; i < nspecs; ++i)
            {
                QPDFPageData& page_data = parsed_specs.at(i);
                if (cur_page < page_data.selected_pages.size())
                {
                    got_pages = true;
                    new_parsed_specs.push_back(
                        QPDFPageData(
                            page_data,
                            page_data.selected_pages.at(cur_page)));
                }
            }
            ++cur_page;
        }
        parsed_specs = new_parsed_specs;
    }

    // Add all the pages from all the files in the order specified.
    // Keep track of any pages from the original file that we are
    // selecting.
    std::set<int> selected_from_orig;
    std::vector<QPDFObjectHandle> new_labels;
    bool any_page_labels = false;
    int out_pageno = 0;
    for (std::vector<QPDFPageData>::iterator iter =
             parsed_specs.begin();
         iter != parsed_specs.end(); ++iter)
    {
        QPDFPageData& page_data = *iter;
        ClosedFileInputSource* cis = 0;
        if (page_spec_cfis.count(page_data.filename))
        {
            cis = page_spec_cfis[page_data.filename];
            cis->stayOpen(true);
        }
        QPDFPageLabelDocumentHelper pldh(*page_data.qpdf);
        if (pldh.hasPageLabels())
        {
            any_page_labels = true;
        }
        if (o.verbose)
        {
            std::cout << whoami << ": adding pages from "
                      << page_data.filename << std::endl;
        }
        for (std::vector<int>::iterator pageno_iter =
                 page_data.selected_pages.begin();
             pageno_iter != page_data.selected_pages.end();
             ++pageno_iter, ++out_pageno)
        {
            // Pages are specified from 1 but numbered from 0 in the
            // vector
            int pageno = *pageno_iter - 1;
            pldh.getLabelsForPageRange(pageno, pageno, out_pageno,
                                       new_labels);
            QPDFPageObjectHelper to_copy = page_data.orig_pages.at(pageno);
            QPDFObjGen to_copy_og = to_copy.getObjectHandle().getObjGen();
            unsigned long long from_uuid = page_data.qpdf->getUniqueId();
            if (copied_pages[from_uuid].count(to_copy_og))
            {
                QTC::TC("qpdf", "qpdf copy same page more than once",
                        (page_data.qpdf == &pdf) ? 0 : 1);
                to_copy = to_copy.shallowCopyPage();
            }
            else
            {
                copied_pages[from_uuid].insert(to_copy_og);
            }
            dh.addPage(to_copy, false);
            if (page_data.qpdf == &pdf)
            {
                // This is a page from the original file. Keep track
                // of the fact that we are using it.
                selected_from_orig.insert(pageno);
            }
        }
        if (cis)
        {
            cis->stayOpen(false);
        }
    }
    if (any_page_labels)
    {
        QPDFObjectHandle page_labels =
            QPDFObjectHandle::newDictionary();
        page_labels.replaceKey(
            "/Nums", QPDFObjectHandle::newArray(new_labels));
        pdf.getRoot().replaceKey("/PageLabels", page_labels);
    }

    // Delete page objects for unused page in primary. This prevents
    // those objects from being preserved by being referred to from
    // other places, such as the outlines dictionary.
    for (size_t pageno = 0; pageno < orig_pages.size(); ++pageno)
    {
        if (selected_from_orig.count(pageno) == 0)
        {
            pdf.replaceObject(
                orig_pages.at(pageno).getObjectHandle().getObjGen(),
                QPDFObjectHandle::newNull());
        }
    }
}

static void handle_rotations(QPDF& pdf, Options& o)
{
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
    int npages = static_cast<int>(pages.size());
    for (std::map<std::string, RotationSpec>::iterator iter =
             o.rotations.begin();
         iter != o.rotations.end(); ++iter)
    {
        std::string const& range = (*iter).first;
        RotationSpec const& rspec = (*iter).second;
        // range has been previously validated
        std::vector<int> to_rotate =
            QUtil::parse_numrange(range.c_str(), npages);
        for (std::vector<int>::iterator i2 = to_rotate.begin();
             i2 != to_rotate.end(); ++i2)
        {
            int pageno = *i2 - 1;
            if ((pageno >= 0) && (pageno < npages))
            {
                pages.at(pageno).rotatePage(rspec.angle, rspec.relative);
            }
        }
    }
}

static void maybe_fix_write_password(int R, Options& o, std::string& password)
{
    switch (o.password_mode)
    {
      case pm_bytes:
        QTC::TC("qpdf", "qpdf password mode bytes");
        break;

      case pm_hex_bytes:
        QTC::TC("qpdf", "qpdf password mode hex-bytes");
        password = QUtil::hex_decode(password);
        break;

      case pm_unicode:
      case pm_auto:
        {
            bool has_8bit_chars;
            bool is_valid_utf8;
            bool is_utf16;
            QUtil::analyze_encoding(password,
                                    has_8bit_chars,
                                    is_valid_utf8,
                                    is_utf16);
            if (! has_8bit_chars)
            {
                return;
            }
            if (o.password_mode == pm_unicode)
            {
                if (! is_valid_utf8)
                {
                    QTC::TC("qpdf", "qpdf password not unicode");
                    throw std::runtime_error(
                        "supplied password is not valid UTF-8");
                }
                if (R < 5)
                {
                    std::string encoded;
                    if (! QUtil::utf8_to_pdf_doc(password, encoded))
                    {
                        QTC::TC("qpdf", "qpdf password not encodable");
                        throw std::runtime_error(
                            "supplied password cannot be encoded for"
                            " 40-bit or 128-bit encryption formats");
                    }
                    password = encoded;
                }
            }
            else
            {
                if ((R < 5) && is_valid_utf8)
                {
                    std::string encoded;
                    if (QUtil::utf8_to_pdf_doc(password, encoded))
                    {
                        QTC::TC("qpdf", "qpdf auto-encode password");
                        if (o.verbose)
                        {
                            std::cout
                                << whoami
                                << ": automatically converting Unicode"
                                << " password to single-byte encoding as"
                                << " required for 40-bit or 128-bit"
                                << " encryption" << std::endl;
                        }
                        password = encoded;
                    }
                    else
                    {
                        QTC::TC("qpdf", "qpdf bytes fallback warning");
                        std::cerr
                            << whoami << ": WARNING: "
                            << "supplied password looks like a Unicode"
                            << " password with characters not allowed in"
                            << " passwords for 40-bit and 128-bit encryption;"
                            << " most readers will not be able to open this"
                            << " file with the supplied password."
                            << " (Use --password-mode=bytes to suppress this"
                            << " warning and use the password anyway.)"
                            << std::endl;
                    }
                }
                else if ((R >= 5) && (! is_valid_utf8))
                {
                    QTC::TC("qpdf", "qpdf invalid utf-8 in auto");
                    throw std::runtime_error(
                        "supplied password is not a valid Unicode password,"
                        " which is required for 256-bit encryption; to"
                        " really use this password, rerun with the"
                        " --password-mode=bytes option");
                }
            }
        }
        break;
    }
}

static void set_encryption_options(QPDF& pdf, Options& o, QPDFWriter& w)
{
    int R = 0;
    if (o.keylen == 40)
    {
        R = 2;
    }
    else if (o.keylen == 128)
    {
        if (o.force_V4 || o.cleartext_metadata || o.use_aes)
        {
            R = 4;
        }
        else
        {
            R = 3;
        }
    }
    else if (o.keylen == 256)
    {
        if (o.force_R5)
        {
            R = 5;
        }
        else
        {
            R = 6;
        }
    }
    else
    {
        throw std::logic_error("bad encryption keylen");
    }
    if ((R > 3) && (o.r3_accessibility == false))
    {
        std::cerr << whoami
                  << ": -accessibility=n is ignored for modern"
                  << " encryption formats" << std::endl;
    }
    maybe_fix_write_password(R, o, o.user_password);
    maybe_fix_write_password(R, o, o.owner_password);
    switch (R)
    {
      case 2:
        w.setR2EncryptionParameters(
            o.user_password.c_str(), o.owner_password.c_str(),
            o.r2_print, o.r2_modify, o.r2_extract, o.r2_annotate);
        break;
      case 3:
        w.setR3EncryptionParameters(
            o.user_password.c_str(), o.owner_password.c_str(),
            o.r3_accessibility, o.r3_extract,
            o.r3_assemble, o.r3_annotate_and_form,
            o.r3_form_filling, o.r3_modify_other,
            o.r3_print);
        break;
      case 4:
        w.setR4EncryptionParameters(
            o.user_password.c_str(), o.owner_password.c_str(),
            o.r3_accessibility, o.r3_extract,
            o.r3_assemble, o.r3_annotate_and_form,
            o.r3_form_filling, o.r3_modify_other,
            o.r3_print, !o.cleartext_metadata, o.use_aes);
        break;
      case 5:
        w.setR5EncryptionParameters(
            o.user_password.c_str(), o.owner_password.c_str(),
            o.r3_accessibility, o.r3_extract,
            o.r3_assemble, o.r3_annotate_and_form,
            o.r3_form_filling, o.r3_modify_other,
            o.r3_print, !o.cleartext_metadata);
        break;
      case 6:
        w.setR6EncryptionParameters(
            o.user_password.c_str(), o.owner_password.c_str(),
            o.r3_accessibility, o.r3_extract,
            o.r3_assemble, o.r3_annotate_and_form,
            o.r3_form_filling, o.r3_modify_other,
            o.r3_print, !o.cleartext_metadata);
        break;
      default:
        throw std::logic_error("bad encryption R value");
        break;
    }
}

static void set_writer_options(QPDF& pdf, Options& o, QPDFWriter& w)
{
    if (o.qdf_mode)
    {
        w.setQDFMode(true);
    }
    if (o.preserve_unreferenced_objects)
    {
        w.setPreserveUnreferencedObjects(true);
    }
    if (o.newline_before_endstream)
    {
        w.setNewlineBeforeEndstream(true);
    }
    if (o.normalize_set)
    {
        w.setContentNormalization(o.normalize);
    }
    if (o.stream_data_set)
    {
        w.setStreamDataMode(o.stream_data_mode);
    }
    if (o.compress_streams_set)
    {
        w.setCompressStreams(o.compress_streams);
    }
    if (o.decode_level_set)
    {
        w.setDecodeLevel(o.decode_level);
    }
    if (o.decrypt)
    {
        w.setPreserveEncryption(false);
    }
    if (o.deterministic_id)
    {
        w.setDeterministicID(true);
    }
    if (o.static_id)
    {
        w.setStaticID(true);
    }
    if (o.static_aes_iv)
    {
        w.setStaticAesIV(true);
    }
    if (o.suppress_original_object_id)
    {
        w.setSuppressOriginalObjectIDs(true);
    }
    if (o.copy_encryption)
    {
        PointerHolder<QPDF> encryption_pdf =
            process_file(
                o.encryption_file, o.encryption_file_password, o);
        w.copyEncryptionParameters(*encryption_pdf);
    }
    if (o.encrypt)
    {
        set_encryption_options(pdf, o, w);
    }
    if (o.linearize)
    {
        w.setLinearization(true);
    }
    if (! o.linearize_pass1.empty())
    {
        w.setLinearizationPass1Filename(o.linearize_pass1);
    }
    if (o.object_stream_set)
    {
        w.setObjectStreamMode(o.object_stream_mode);
    }
    if (! o.min_version.empty())
    {
        std::string version;
        int extension_level = 0;
        parse_version(o.min_version, version, extension_level);
        w.setMinimumPDFVersion(version, extension_level);
    }
    if (! o.force_version.empty())
    {
        std::string version;
        int extension_level = 0;
        parse_version(o.force_version, version, extension_level);
        w.forcePDFVersion(version, extension_level);
    }
    if (o.progress && o.outfilename)
    {
        w.registerProgressReporter(new ProgressReporter(o.outfilename));
    }
}

static void write_outfile(QPDF& pdf, Options& o)
{
    if (o.split_pages)
    {
        // Generate output file pattern
        std::string before;
        std::string after;
        size_t len = strlen(o.outfilename);
        char* num_spot = strstr(const_cast<char*>(o.outfilename), "%d");
        if (num_spot != 0)
        {
            QTC::TC("qpdf", "qpdf split-pages %d");
            before = std::string(o.outfilename, (num_spot - o.outfilename));
            after = num_spot + 2;
        }
        else if ((len >= 4) &&
                 (QUtil::strcasecmp(o.outfilename + len - 4, ".pdf") == 0))
        {
            QTC::TC("qpdf", "qpdf split-pages .pdf");
            before = std::string(o.outfilename, len - 4) + "-";
            after = o.outfilename + len - 4;
        }
        else
        {
            QTC::TC("qpdf", "qpdf split-pages other");
            before = std::string(o.outfilename) + "-";
        }

        if (! o.preserve_unreferenced_page_resources)
        {
            QPDFPageDocumentHelper dh(pdf);
            dh.removeUnreferencedResources();
        }
        QPDFPageLabelDocumentHelper pldh(pdf);
        std::vector<QPDFObjectHandle> const& pages = pdf.getAllPages();
        int pageno_len = QUtil::int_to_string(pages.size()).length();
        unsigned int num_pages = pages.size();
        for (unsigned int i = 0; i < num_pages; i += o.split_pages)
        {
            unsigned int first = i + 1;
            unsigned int last = i + o.split_pages;
            if (last > num_pages)
            {
                last = num_pages;
            }
            QPDF outpdf;
            outpdf.emptyPDF();
            for (unsigned int pageno = first; pageno <= last; ++pageno)
            {
                QPDFObjectHandle page = pages.at(pageno - 1);
                outpdf.addPage(page, false);
            }
            if (pldh.hasPageLabels())
            {
                std::vector<QPDFObjectHandle> labels;
                pldh.getLabelsForPageRange(first - 1, last - 1, 0, labels);
                QPDFObjectHandle page_labels =
                    QPDFObjectHandle::newDictionary();
                page_labels.replaceKey(
                    "/Nums", QPDFObjectHandle::newArray(labels));
                outpdf.getRoot().replaceKey("/PageLabels", page_labels);
            }
            // Copying the outlines tree, names table, and any
            // outdated Dests key from the original file will make
            // some things work in the split files. It is not a
            // complete solution, but at least outlines whose
            // destinations are on pages that have been preserved will
            // work normally. There are other top-level structures
            // that should be copied as well. This will be improved in
            // the future.
            std::list<std::string> to_copy;
            to_copy.push_back("/Names");
            to_copy.push_back("/Dests");
            to_copy.push_back("/Outlines");
            for (std::list<std::string>::iterator iter = to_copy.begin();
                 iter != to_copy.end(); ++iter)
            {
                QPDFObjectHandle orig = pdf.getRoot().getKey(*iter);
                if (! orig.isIndirect())
                {
                    orig = pdf.makeIndirectObject(orig);
                }
                outpdf.getRoot().replaceKey(
                    *iter,
                    outpdf.copyForeignObject(orig));
            }
            std::string page_range = QUtil::int_to_string(first, pageno_len);
            if (o.split_pages > 1)
            {
                page_range += "-" + QUtil::int_to_string(last, pageno_len);
            }
            std::string outfile = before + page_range + after;
            QPDFWriter w(outpdf, outfile.c_str());
            set_writer_options(outpdf, o, w);
            w.write();
            if (o.verbose)
            {
                std::cout << whoami << ": wrote file " << outfile << std::endl;
            }
        }
    }
    else
    {
        if (strcmp(o.outfilename, "-") == 0)
        {
            o.outfilename = 0;
        }
        QPDFWriter w(pdf, o.outfilename);
        set_writer_options(pdf, o, w);
        w.write();
        if (o.verbose)
        {
            std::cout << whoami << ": wrote file "
                      << o.outfilename << std::endl;
        }
    }
}

int realmain(int argc, char* argv[])
{
    whoami = QUtil::getWhoami(argv[0]);
    QUtil::setLineBuf(stdout);

    // Remove prefix added by libtool for consistency during testing.
    if (strncmp(whoami, "lt-", 3) == 0)
    {
	whoami += 3;
    }

    // ArgParser must stay in scope for the duration of qpdf's run as
    // it holds dynamic memory used for argv.
    Options o;
    ArgParser ap(argc, argv, o);
    ap.parseOptions();

    try
    {
	PointerHolder<QPDF> pdf_ph =
            process_file(o.infilename, o.password, o);
        QPDF& pdf = *pdf_ph;
        if (! o.page_specs.empty())
        {
            handle_page_specs(pdf, o);
        }
        if (! o.rotations.empty())
        {
            handle_rotations(pdf, o);
        }
        handle_under_overlay(pdf, o);
        handle_transformations(pdf, o);

	if (o.outfilename == 0)
	{
            do_inspection(pdf, o);
	}
	else
	{
            write_outfile(pdf, o);
	}
	if (! pdf.getWarnings().empty())
	{
            if (! o.suppress_warnings)
            {
                std::cerr << whoami << ": operation succeeded with warnings;"
                          << " resulting file may have some problems"
                          << std::endl;
            }
            // Still exit with warning code even if warnings were suppressed.
            exit(EXIT_WARNING);
	}
    }
    catch (std::exception& e)
    {
	std::cerr << e.what() << std::endl;
	exit(EXIT_ERROR);
    }

    return 0;
}

#ifdef WINDOWS_WMAIN

extern "C"
int wmain(int argc, wchar_t* argv[])
{
    // If wmain is supported, argv contains UTF-16-encoded strings
    // with a 16-bit wchar_t. Convert this to UTF-8-encoded strings
    // for compatibility with other systems. That way the rest of
    // qpdf.cc can just act like arguments are UTF-8.
    std::vector<PointerHolder<char> > utf8_argv;
    for (int i = 0; i < argc; ++i)
    {
        std::string utf16;
        for (size_t j = 0; j < wcslen(argv[i]); ++j)
        {
            unsigned short codepoint = static_cast<unsigned short>(argv[i][j]);
            utf16.append(1, static_cast<unsigned char>(codepoint >> 8));
            utf16.append(1, static_cast<unsigned char>(codepoint & 0xff));
        }
        std::string utf8 = QUtil::utf16_to_utf8(utf16);
        utf8_argv.push_back(
            PointerHolder<char>(true, QUtil::copy_string(utf8.c_str())));
    }
    PointerHolder<char*> utf8_argv_ph =
        PointerHolder<char*>(true, new char*[1+utf8_argv.size()]);
    char** new_argv = utf8_argv_ph.getPointer();
    for (size_t i = 0; i < utf8_argv.size(); ++i)
    {
        new_argv[i] = utf8_argv.at(i).getPointer();
    }
    argc = static_cast<int>(utf8_argv.size());
    new_argv[argc] = 0;
    return realmain(argc, new_argv);
}

#else

int main(int argc, char* argv[])
{
    return realmain(argc, argv);
}

#endif
