/*
** 2003 September 6
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This is the header file for information that is private to the
** VDBE.  This information used to all be at the top of the single
** source code file "vdbe.c".  When that file became too big (over
** 6000 lines long) it was split up into several smaller files and
** this header information was factored out.
*/

/*
** The makefile scans this source file and creates the following
** array of string constants which are the names of all VDBE opcodes.
** This array is defined in a separate source code file named opcode.c
** which is automatically generated by the makefile.
*/
extern char *sqliteOpcodeNames[];

/*
** SQL is translated into a sequence of instructions to be
** executed by a virtual machine.  Each instruction is an instance
** of the following structure.
*/
typedef struct VdbeOp Op;

/*
** Boolean values
*/
typedef unsigned char Bool;

/*
** A cursor is a pointer into a single BTree within a database file.
** The cursor can seek to a BTree entry with a particular key, or
** loop over all entries of the Btree.  You can also insert new BTree
** entries or retrieve the key or data from the entry that the cursor
** is currently pointing to.
** 
** Every cursor that the virtual machine has open is represented by an
** instance of the following structure.
**
** If the Cursor.isTriggerRow flag is set it means that this cursor is
** really a single row that represents the NEW or OLD pseudo-table of
** a row trigger.  The data for the row is stored in Cursor.pData and
** the rowid is in Cursor.iKey.
*/
struct Cursor {
  BtCursor *pCursor;    /* The cursor structure of the backend */
  int lastRecno;        /* Last recno from a Next or NextIdx operation */
  int nextRowid;        /* Next rowid returned by OP_NewRowid */
  Bool recnoIsValid;    /* True if lastRecno is valid */
  Bool keyAsData;       /* The OP_Column command works on key instead of data */
  Bool atFirst;         /* True if pointing to first entry */
  Bool useRandomRowid;  /* Generate new record numbers semi-randomly */
  Bool nullRow;         /* True if pointing to a row with no data */
  Bool nextRowidValid;  /* True if the nextRowid field is valid */
  Bool pseudoTable;     /* This is a NEW or OLD pseudo-tables of a trigger */
  Btree *pBt;           /* Separate file holding temporary table */
  int nData;            /* Number of bytes in pData */
  char *pData;          /* Data for a NEW or OLD pseudo-table */
  int iKey;             /* Key for the NEW or OLD pseudo-table row */
};
typedef struct Cursor Cursor;

/*
** A sorter builds a list of elements to be sorted.  Each element of
** the list is an instance of the following structure.
*/
typedef struct Sorter Sorter;
struct Sorter {
  int nKey;           /* Number of bytes in the key */
  char *zKey;         /* The key by which we will sort */
  int nData;          /* Number of bytes in the data */
  char *pData;        /* The data associated with this key */
  Sorter *pNext;      /* Next in the list */
};

/* 
** Number of buckets used for merge-sort.  
*/
#define NSORT 30

/*
** Number of bytes of string storage space available to each stack
** layer without having to malloc.  NBFS is short for Number of Bytes
** For Strings.
*/
#define NBFS 32

/*
** A single level of the stack is an instance of the following
** structure.  Except, string values are stored on a separate
** list of of pointers to character.  The reason for storing
** strings separately is so that they can be easily passed
** to the callback function.
*/
struct Stack {
  int i;         /* Integer value */
  int n;         /* Number of characters in string value, including '\0' */
  int flags;     /* Some combination of STK_Null, STK_Str, STK_Dyn, etc. */
  double r;      /* Real value */
  char z[NBFS];  /* Space for short strings */
};
typedef struct Stack Stack;

/*
** Memory cells use the same structure as the stack except that space
** for an arbitrary string is added.
*/
struct Mem {
  Stack s;       /* All values of the memory cell besides string */
  char *z;       /* String value for this memory cell */
};
typedef struct Mem Mem;

/*
** Allowed values for Stack.flags
*/
#define STK_Null      0x0001   /* Value is NULL */
#define STK_Str       0x0002   /* Value is a string */
#define STK_Int       0x0004   /* Value is an integer */
#define STK_Real      0x0008   /* Value is a real number */
#define STK_Dyn       0x0010   /* Need to call sqliteFree() on zStack[] */
#define STK_Static    0x0020   /* zStack[] points to a static string */
#define STK_Ephem     0x0040   /* zStack[] points to an ephemeral string */

/* The following STK_ value appears only in AggElem.aMem.s.flag fields.
** It indicates that the corresponding AggElem.aMem.z points to a
** aggregate function context that needs to be finalized.
*/
#define STK_AggCtx    0x0040   /* zStack[] points to an agg function context */

/*
** The "context" argument for a installable function.  A pointer to an
** instance of this structure is the first argument to the routines used
** implement the SQL functions.
**
** There is a typedef for this structure in sqlite.h.  So all routines,
** even the public interface to SQLite, can use a pointer to this structure.
** But this file is the only place where the internal details of this
** structure are known.
**
** This structure is defined inside of vdbe.c because it uses substructures
** (Stack) which are only defined there.
*/
struct sqlite_func {
  FuncDef *pFunc;   /* Pointer to function information.  MUST BE FIRST */
  Stack s;          /* Small strings, ints, and double values go here */
  char *z;          /* Space for holding dynamic string results */
  void *pAgg;       /* Aggregate context */
  u8 isError;       /* Set to true for an error */
  u8 isStep;        /* Current in the step function */
  int cnt;          /* Number of times that the step function has been called */
};

/*
** An Agg structure describes an Aggregator.  Each Agg consists of
** zero or more Aggregator elements (AggElem).  Each AggElem contains
** a key and one or more values.  The values are used in processing
** aggregate functions in a SELECT.  The key is used to implement
** the GROUP BY clause of a select.
*/
typedef struct Agg Agg;
typedef struct AggElem AggElem;
struct Agg {
  int nMem;            /* Number of values stored in each AggElem */
  AggElem *pCurrent;   /* The AggElem currently in focus */
  HashElem *pSearch;   /* The hash element for pCurrent */
  Hash hash;           /* Hash table of all aggregate elements */
  FuncDef **apFunc;    /* Information about aggregate functions */
};
struct AggElem {
  char *zKey;          /* The key to this AggElem */
  int nKey;            /* Number of bytes in the key, including '\0' at end */
  Mem aMem[1];         /* The values for this AggElem */
};

/*
** A Set structure is used for quick testing to see if a value
** is part of a small set.  Sets are used to implement code like
** this:
**            x.y IN ('hi','hoo','hum')
*/
typedef struct Set Set;
struct Set {
  Hash hash;             /* A set is just a hash table */
  HashElem *prev;        /* Previously accessed hash elemen */
};

/*
** A Keylist is a bunch of keys into a table.  The keylist can
** grow without bound.  The keylist stores the ROWIDs of database
** records that need to be deleted or updated.
*/
typedef struct Keylist Keylist;
struct Keylist {
  int nKey;         /* Number of slots in aKey[] */
  int nUsed;        /* Next unwritten slot in aKey[] */
  int nRead;        /* Next unread slot in aKey[] */
  Keylist *pNext;   /* Next block of keys */
  int aKey[1];      /* One or more keys.  Extra space allocated as needed */
};

/*
** An instance of the virtual machine.  This structure contains the complete
** state of the virtual machine.
**
** The "sqlite_vm" structure pointer that is returned by sqlite_compile()
** is really a pointer to an instance of this structure.
*/
struct Vdbe {
  sqlite *db;         /* The whole database */
  Vdbe *pPrev,*pNext; /* Linked list of VDBEs with the same Vdbe.db */
  FILE *trace;        /* Write an execution trace here, if not NULL */
  int nOp;            /* Number of instructions in the program */
  int nOpAlloc;       /* Number of slots allocated for aOp[] */
  Op *aOp;            /* Space to hold the virtual machine's program */
  int nLabel;         /* Number of labels used */
  int nLabelAlloc;    /* Number of slots allocated in aLabel[] */
  int *aLabel;        /* Space to hold the labels */
  int tos;            /* Index of top of stack */
  Stack *aStack;      /* The operand stack, except string values */
  char **zStack;      /* Text or binary values of the stack */
  char **azColName;   /* Becomes the 4th parameter to callbacks */
  int nCursor;        /* Number of slots in aCsr[] */
  Cursor *aCsr;       /* One element of this array for each open cursor */
  Sorter *pSort;      /* A linked list of objects to be sorted */
  FILE *pFile;        /* At most one open file handler */
  int nField;         /* Number of file fields */
  char **azField;     /* Data for each file field */
  int nVariable;          /* Number of entries in azVariable[] */
  char **azVariable;      /* Values for the OP_Variable opcode */
  char *zLine;            /* A single line from the input file */
  int nLineAlloc;         /* Number of spaces allocated for zLine */
  int magic;              /* Magic number for sanity checking */
  int nMem;               /* Number of memory locations currently allocated */
  Mem *aMem;              /* The memory locations */
  Agg agg;                /* Aggregate information */
  int nSet;               /* Number of sets allocated */
  Set *aSet;              /* An array of sets */
  int nCallback;          /* Number of callbacks invoked so far */
  Keylist *pList;         /* A list of ROWIDs */
  int keylistStackDepth;  /* The size of the "keylist" stack */
  Keylist **keylistStack; /* The stack used by opcodes ListPush & ListPop */
  int pc;                 /* The program counter */
  int rc;                 /* Value to return */
  unsigned uniqueCnt;     /* Used by OP_MakeRecord when P2!=0 */
  int errorAction;        /* Recovery action to do in case of an error */
  int undoTransOnError;   /* If error, either ROLLBACK or COMMIT */
  int inTempTrans;        /* True if temp database is transactioned */
  int returnStack[100];   /* Return address stack for OP_Gosub & OP_Return */
  int returnDepth;        /* Next unused element in returnStack[] */
  int nResColumn;         /* Number of columns in one row of the result set */
  char **azResColumn;                        /* Values for one row of result */ 
  int (*xCallback)(void*,int,char**,char**); /* Callback for SELECT results */
  void *pCbArg;                              /* First argument to xCallback() */
  int popStack;           /* Pop the stack this much on entry to VdbeExec() */
  char *zErrMsg;          /* Error message written here */
  u8 explain;             /* True if EXPLAIN present on SQL command */
};

/*
** The following are allowed values for Vdbe.magic
*/
#define VDBE_MAGIC_INIT     0x26bceaa5    /* Building a VDBE program */
#define VDBE_MAGIC_RUN      0xbdf20da3    /* VDBE is ready to execute */
#define VDBE_MAGIC_HALT     0x519c2973    /* VDBE has completed execution */
#define VDBE_MAGIC_DEAD     0xb606c3c8    /* The VDBE has been deallocated */

/*
** Here is a macro to handle the common case of popping the stack
** once.  This macro only works from within the sqliteVdbeExec()
** function.
*/
#define POPSTACK \
  assert(p->tos>=0); \
  if( aStack[p->tos].flags & STK_Dyn ) sqliteFree(zStack[p->tos]); \
  p->tos--;

/*
** Function prototypes
*/
void sqliteVdbeCleanupCursor(Cursor*);
void sqliteVdbeSorterReset(Vdbe*);
void sqliteVdbeAggReset(Agg*);
void sqliteVdbeKeylistFree(Keylist*);
void sqliteVdbePopStack(Vdbe*,int);
#if !defined(NDEBUG) || defined(VDBE_PROFILE)
void sqliteVdbePrintOp(FILE*, int, Op*);
#endif
