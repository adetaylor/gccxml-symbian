

/*--------------------------------------------------------------------------*/
/* Data structures for the actual XML dump.  */

#ifndef GCCXML_XML_HEADER
#define GCCXML_XML_HEADER

/* A "dump node" corresponding to a particular tree node.  */
typedef struct xml_dump_node
{
  /* The index for the node.  */
  unsigned int index;

  /* Whether the node is complete.  */
  unsigned int complete;
} *xml_dump_node_p;

/* A node on the queue of dump nodes.  */
typedef struct xml_dump_queue
{
  /* The queued tree node.  */
  tree tree_node;

  /* The corresponding dump node.  */
  xml_dump_node_p dump_node;

  /* The next node in the queue.  */
  struct xml_dump_queue *next;
} *xml_dump_queue_p;

/* A node on the queue of file names.  */
typedef struct xml_file_queue
{
  /* The queued file name.  */
  splay_tree_node tree_node;

  /* The next node in the queue.  */
  struct xml_file_queue *next;
} *xml_file_queue_p;

/* Dump control structure.  A pointer one instance of this is passed
   to nearly every function.  */
typedef struct xml_dump_info
{
  /* Output file stream of dump.  */
  FILE* file;

  /* Which pass of the loop we are doing (1=complete or 0=incomplete).  */
  int require_complete;

  /* Next available index to identify node.  */
  unsigned int next_index;

  /* The first node in the queue of complete nodes.  */
  xml_dump_queue_p queue;

  /* The last node in the queue of complete nodes.  */
  xml_dump_queue_p queue_end;

  /* List of free xml_dump_queue nodes.  */
  xml_dump_queue_p queue_free;

  /* All nodes that have been encountered.  */
  splay_tree dump_nodes;

  /* Index of the next available file queue position.  */
  unsigned int file_index;

  /* The first file in the queue of filenames.  */
  xml_file_queue_p file_queue;

  /* The last file in the queue of filenames.  */
  xml_file_queue_p file_queue_end;

  /* All files that have been queued.  */
  splay_tree file_nodes;
} *xml_dump_info_p;


/*--------------------------------------------------------------------------*/
/* Data structures for generating documentation.  */

typedef struct xml_document_subelement_s xml_document_subelement;
typedef struct xml_document_attribute_s xml_document_attribute;
typedef struct xml_document_element_s xml_document_element;
typedef struct xml_document_info_s xml_document_info;
typedef xml_document_subelement* xml_document_subelement_p;
typedef xml_document_attribute* xml_document_attribute_p;
typedef xml_document_element* xml_document_element_p;
typedef xml_document_info* xml_document_info_p;

/* Maximum document specification sizes.  */
enum { xml_document_max_elements = 256,
       xml_document_max_attributes = 64,
       xml_document_max_subelements = 256 };

/* Document format types.  */
typedef enum xml_document_format_e
{ xml_document_format_dtd,
  xml_document_format_schema
} xml_document_format;

/* The modes of attribute use.  */
typedef enum xml_document_attribute_use_e
{ xml_document_attribute_use_optional,
  xml_document_attribute_use_required
} xml_document_attribute_use;
static const char* xml_document_dtd_uses[] = {"IMPLIED", "REQUIRED"};
static const char* xml_document_schema_uses[] = {"optional", "required"};

/* The set of attribute types.  */
typedef enum xml_document_attribute_type_e
{ xml_document_attribute_type_id,
  xml_document_attribute_type_idref,
  xml_document_attribute_type_idrefs,
  xml_document_attribute_type_integer,
  xml_document_attribute_type_boolean,
  xml_document_attribute_type_string,
  xml_document_attribute_type_enum_access
} xml_document_attribute_type;
static const char* xml_document_dtd_types[] =
{"ID", "IDREF", "IDREFS", "CDATA", "CDATA", "CDATA",
 "(public|protected|private)"};
static const char* xml_document_schema_types[] =
{"xs:ID", "xs:IDREF", "xs:IDREFS", "xs:integer", "xs:boolean", "xs:string",
 "????"};

/* Represent one element attribute.  */
struct xml_document_attribute_s
{
  /* The name of the attribute.  */
  const char* name;

  /* The type of the attribute.  */
  xml_document_attribute_type type;

  /* Usage requirements in the containing element (optional, required).  */
  xml_document_attribute_use use;

  /* The value of the attribute, if any.  When the usage is required
     this specifies the required value.  When the usage is optional
     this specifies the default value.  */
  const char* value;
};

/* Represent a reference to a nested element.  */
struct xml_document_subelement_s
{
  /* The element that may be nested.  */
  xml_document_element_p element;

  /* Whether the subelement is required to be present (at least once).  */
  int required;

  /* Whether the subelement is allowed to be repeated (more than once).  */
  int repeatable;
};

/* Represent an element specification.  */
struct xml_document_element_s
{
  /* The name of the element.  */
  const char* name;

  /* The attribute specification.  */
  int num_attributes;
  xml_document_attribute attributes[xml_document_max_attributes];

  /* The subelement specification.  */
  int num_subelements;
  xml_document_subelement subelements[xml_document_max_subelements];
};

/* Represent a full document specification.  */
struct xml_document_info_s
{
  /* The set of allowed elements.  The first one is the root which
     references the others.  */
  int num_elements;
  xml_document_element elements[xml_document_max_elements];

  /* The format of the documentation to be generated.  */
  xml_document_format format;

  /* Output file stream for document.  */
  FILE* file;
};

static void
xml_document_add_attribute(xml_document_element_p element,
                           const char* name,
                           xml_document_attribute_type type,
                           xml_document_attribute_use use,
                           const char* value)
{
  xml_document_attribute_p attribute =
    &element->attributes[element->num_attributes++];
  attribute->name = name;
  attribute->type = type;
  attribute->use = use;
  attribute->value = value;
}

static xml_document_element_p
xml_document_add_subelement(xml_document_info_p xdi,
                            xml_document_element_p parent,
                            int required, int repeatable)
{
  xml_document_element_p element = &xdi->elements[xdi->num_elements++];
  xml_document_subelement_p subelement =
    &parent->subelements[parent->num_subelements++];
  subelement->element = element;
  subelement->required = required;
  subelement->repeatable = repeatable;
  return element;
}

/*--------------------------------------------------------------------------*/
/* Dump utility declarations.  */

void do_xml_output PARAMS ((const char*));
void do_xml_document PARAMS ((const char*, const char*));

extern int xml_add_node PARAMS((xml_dump_info_p, tree, int));
static void xml_dump PARAMS((xml_dump_info_p));
static int xml_queue_incomplete_dump_nodes PARAMS((splay_tree_node, void*));
static void xml_dump_tree_node PARAMS((xml_dump_info_p, tree, xml_dump_node_p));
static void xml_dump_files PARAMS((xml_dump_info_p));

static void xml_add_start_nodes PARAMS((xml_dump_info_p, const char*));

static const char* xml_get_encoded_string PARAMS ((tree));
static const char* xml_get_encoded_string_from_string PARAMS ((const char*));
static tree xml_get_encoded_identifier_from_string PARAMS ((const char*));
static const char* xml_escape_string PARAMS ((const char* in_str));
static int xml_fill_all_decls(struct cpp_reader*, hashnode, const void*);
#endif
