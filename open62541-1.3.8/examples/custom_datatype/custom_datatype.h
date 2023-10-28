/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

typedef struct {
    UA_Float x;
    UA_Float y;
    UA_Float z;
} Point;

/* The datatype description for the Point datatype */
#define Point_padding_y offsetof(Point,y) - offsetof(Point,x) - sizeof(UA_Float)
#define Point_padding_z offsetof(Point,z) - offsetof(Point,y) - sizeof(UA_Float)

/* The binary encoding id's for the datatypes */
#define Point_binary_encoding_id        1
#define Measurement_binary_encoding_id  2
#define Opt_binary_encoding_id          3
#define Uni_binary_encoding_id          4


static UA_DataTypeMember Point_members[3] = {
    /* x */
    {
        UA_TYPENAME("x")           /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        0,                         /* .padding */
        false,                     /* .isArray */
        false                      /* .isOptional */
    },
    /* y */
    {
        UA_TYPENAME("y")           /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        Point_padding_y,           /* .padding */
        false,                     /* .isArray */
        false                      /* .isOptional */
    },
    /* z */
    {
        UA_TYPENAME("z")           /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        Point_padding_z,           /* .padding */
        false,                     /* .isArray */
        false                      /* .isOptional */
    }
};

static const UA_DataType PointType = {
        UA_TYPENAME("Point")                /* .tyspeName */
        {1, UA_NODEIDTYPE_NUMERIC, {4242}}, /* .typeId */
        {1, UA_NODEIDTYPE_NUMERIC, {Point_binary_encoding_id}}, /* .binaryEncodingId, the numeric
                                            identifier used on the wire (the
                                            namespaceindex is from .typeId) */
        sizeof(Point),                      /* .memSize */
        UA_DATATYPEKIND_STRUCTURE,          /* .typeKind */
        true,                               /* .pointerFree */
        false,                              /* .overlayable (depends on endianness and
                                            the absence of padding) */
        3,                                  /* .membersSize */
        Point_members
};

/* The datatype description for the Measurement-Series datatype (Array Example)*/
typedef struct {
    UA_String description;
    size_t measurementSize;
    UA_Float *measurement;
} Measurements;

static UA_DataTypeMember Measurements_members[2] = {
    {
        UA_TYPENAME("Measurement description") /* .memberName */
        &UA_TYPES[UA_TYPES_STRING],            /* .memberType */
        0,                                     /* .padding */
        false,                                 /* .isArray */
        false                                  /* .isOptional */
    },
    {
        UA_TYPENAME("Measurements")            /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT],             /* .memberType */
        0,                                     /* .padding */
        true,                                  /* .isArray */
        false                                  /* .isOptional */
    }
};

static const UA_DataType MeasurementType = {
    UA_TYPENAME("Measurement")              /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4443}},     /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {Measurement_binary_encoding_id}}, /* .binaryEncodingId, the numeric
                                            identifier used on the wire (the
                                            namespaceindex is from .typeId) */
    sizeof(Measurements),                   /* .memSize */
    UA_DATATYPEKIND_STRUCTURE,              /* .typeKind */
    false,                                  /* .pointerFree */
    false,                                  /* .overlayable (depends on endianness and
                                                the absence of padding) */
    2,                                      /* .membersSize */
    Measurements_members
};


/* The datatype description for the Opt datatype (Structure with optional fields example)*/
typedef struct {
    UA_Int16 a;
    UA_Float *b;
    UA_Float *c;
} Opt;

static UA_DataTypeMember Opt_members[3] = {
    /* a */
    {
        UA_TYPENAME("a")                                      /* .memberName */
        &UA_TYPES[UA_TYPES_INT16],                            /* .memberType */
        0,                                                    /* .padding */
        false,                                                /* .isArray */
        false                                                 /* .isOptional */
    },
    /* b */
    {
        UA_TYPENAME("b")                                      /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT],                            /* .memberType */
        offsetof(Opt,b) - offsetof(Opt,a) - sizeof(UA_Int16), /* .padding */
        false,                                                /* .isArray */
        true                                                  /* .isOptional */
    },
    /* c */
    {
        UA_TYPENAME("c")                                      /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT],                            /* .memberType */
        offsetof(Opt,c) - offsetof(Opt,b) - sizeof(void *),   /* .padding */
        false,                                                /* .isArray */
        true                                                  /* .isOptional */
    }
};

static const UA_DataType OptType = {
    UA_TYPENAME("Opt")                  /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4644}}, /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {Opt_binary_encoding_id}}, /* .binaryEncodingId, the numeric
                                        identifier used on the wire (the
                                        namespaceindex is from .typeId) */
    sizeof(Opt),                        /* .memSize */
    UA_DATATYPEKIND_OPTSTRUCT,          /* .typeKind */
    false,                              /* .pointerFree */
    false,                              /* .overlayable (depends on endianness and
                                            the absence of padding) */
    3,                                  /* .membersSize */
    Opt_members
};

/* The datatype description for the Uni datatype (Union example) */
typedef enum {UA_UNISWITCH_NONE = 0, UA_UNISWITCH_OPTIONA = 1, UA_UNISWITCH_OPTIONB = 2} UA_UniSwitch;

typedef struct {
    UA_UniSwitch switchField;
    union {
        UA_Double optionA;
        UA_String optionB;
    } fields;
} Uni;

static UA_DataTypeMember Uni_members[2] = {
    {
        UA_TYPENAME("optionA")         /* .memberName */
        &UA_TYPES[UA_TYPES_DOUBLE],    /* .memberType */
        offsetof(Uni, fields.optionA), /* .padding */
        false,                         /* .isArray */
        false                          /* .isOptional */
    },
    {
        UA_TYPENAME("optionB")         /* .memberName */
        &UA_TYPES[UA_TYPES_STRING],    /* .memberType */
        offsetof(Uni, fields.optionB), /* .padding */
        false,                         /* .isArray */
        false                          /* .isOptional */
    }
};

static const UA_DataType UniType = {
    UA_TYPENAME("Uni")                      /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4845}},     /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {Uni_binary_encoding_id}}, /* .binaryEncodingId, the numeric
                                            identifier used on the wire (the
                                            namespaceindex is from .typeId) */
    sizeof(Uni),                            /* .memSize */
    UA_DATATYPEKIND_UNION,                  /* .typeKind */
    false,                                  /* .pointerFree */
    false,                                  /* .overlayable (depends on endianness and
                                            the absence of padding) */
    2,                                      /* .membersSize */
    Uni_members
};
