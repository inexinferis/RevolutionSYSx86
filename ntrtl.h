/*++ BUILD Version: 0005    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

You may only use this code if you agree to the terms of the Windows Research Kernel Source Code License agreement (see License.txt).
If you do not agree to the terms, do not use the code.


Module Name:

    ntrtl.h

Abstract:

    Include file for NT runtime routines that are callable by both
    kernel mode code in the executive and user mode code in various
    NT subsystems.

--*/

#ifndef _NTRTL_
#define _NTRTL_

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4514)
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif
#if (_MSC_VER > 1020)
#pragma once
#endif
#endif

// begin_ntddk begin_wdm begin_winnt begin_ntifs begin_nthal
//
// for move macros
//
#include <string.h>

// end_ntddk end_wdm end_winnt end_ntifs end_nthal

#ifdef __cplusplus
extern "C" {
#endif

//
// Inverted runtime function table support.
//
// These routines are called by kernel and user code and are not exported.
//

#if defined(_AMD64_)

#define MAXIMUM_INVERTED_FUNCTION_TABLE_SIZE 160

typedef struct _INVERTED_FUNCTION_TABLE_ENTRY {
    PRUNTIME_FUNCTION FunctionTable;
    PVOID ImageBase;
    ULONG SizeOfImage;
    ULONG SizeOfTable;
} INVERTED_FUNCTION_TABLE_ENTRY, *PINVERTED_FUNCTION_TABLE_ENTRY;

typedef struct _INVERTED_FUNCTION_TABLE {
    ULONG CurrentSize;
    ULONG MaximumSize;
    BOOLEAN Overflow;
    INVERTED_FUNCTION_TABLE_ENTRY TableEntry[MAXIMUM_INVERTED_FUNCTION_TABLE_SIZE];
} INVERTED_FUNCTION_TABLE, *PINVERTED_FUNCTION_TABLE;

VOID
RtlInsertInvertedFunctionTable (
    PINVERTED_FUNCTION_TABLE InvertedTable,
    PVOID ImageBase,
    ULONG SizeOfImage
    );

VOID
RtlRemoveInvertedFunctionTable (
    PINVERTED_FUNCTION_TABLE InvertedTable,
    PVOID ImageBase
    );

#endif // defined(_AMD64_)

//
// Define interlocked sequenced list structure.
//
// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis begin_ntosp begin_winnt

#ifndef _SLIST_HEADER_
#define _SLIST_HEADER_

#if defined(_WIN64)

//
// The type SINGLE_LIST_ENTRY is not suitable for use with SLISTs.  For
// WIN64, an entry on an SLIST is required to be 16-byte aligned, while a
// SINGLE_LIST_ENTRY structure has only 8 byte alignment.
//
// Therefore, all SLIST code should use the SLIST_ENTRY type instead of the
// SINGLE_LIST_ENTRY type.
//

#pragma warning(push)
#pragma warning(disable:4324)   // structure padded due to align()
typedef struct DECLSPEC_ALIGN(16) _SLIST_ENTRY *PSLIST_ENTRY;
typedef struct DECLSPEC_ALIGN(16) _SLIST_ENTRY {
    PSLIST_ENTRY Next;
} SLIST_ENTRY;
#pragma warning(pop)

#else

#define SLIST_ENTRY SINGLE_LIST_ENTRY
#define _SLIST_ENTRY _SINGLE_LIST_ENTRY
#define PSLIST_ENTRY PSINGLE_LIST_ENTRY

#endif

#if defined(_WIN64)

typedef struct DECLSPEC_ALIGN(16) _SLIST_HEADER {
    ULONGLONG Alignment;
    ULONGLONG Region;
} SLIST_HEADER;

typedef struct _SLIST_HEADER *PSLIST_HEADER;

#else

typedef union _SLIST_HEADER {
    ULONGLONG Alignment;
    struct {
        SLIST_ENTRY Next;
        USHORT Depth;
        USHORT Sequence;
    };
} SLIST_HEADER, *PSLIST_HEADER;

#endif

#endif

// end_ntddk end_wdm end_nthal end_ntifs end_ntndis end_ntosp end_winnt

VOID
RtlMakeStackTraceDataPresent(
    VOID
    );

// begin_winnt

NTSYSAPI
VOID
NTAPI
RtlInitializeSListHead (
    IN PSLIST_HEADER ListHead
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlFirstEntrySList (
    IN const SLIST_HEADER *ListHead
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

NTSYSAPI
USHORT
NTAPI
RtlQueryDepthSList (
    IN PSLIST_HEADER ListHead
    );

// end_winnt

PSLIST_ENTRY
FASTCALL
RtlInterlockedPushListSList (
     IN PSLIST_HEADER ListHead,
     IN PSLIST_ENTRY List,
     IN PSLIST_ENTRY ListEnd,
     IN ULONG Count
     );


// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis
//
// If debugging support enabled, define an ASSERT macro that works.  Otherwise
// define the ASSERT macro to expand to an empty expression.
//
// The ASSERT macro has been updated to be an expression instead of a statement.
//

NTSYSAPI
VOID
NTAPI
RtlAssert(
    __in PVOID VoidFailedAssertion,
    __in PVOID VoidFileName,
    __in ULONG LineNumber,
    __in_opt PSTR MutableMessage
    );

#if DBG

#define ASSERT( exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
        TRUE)

#define ASSERTMSG( msg, exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, msg ),FALSE) : \
        TRUE)

#define RTL_SOFT_ASSERT(_exp) \
    ((!(_exp)) ? \
        (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #_exp),FALSE) : \
        TRUE)

#define RTL_SOFT_ASSERTMSG(_msg, _exp) \
    ((!(_exp)) ? \
        (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #_exp, (_msg)),FALSE) : \
        TRUE)

#if _MSC_VER >= 1300

#define NT_ASSERT(_exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L#_exp), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

#define NT_ASSERTMSG(_msg, _exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L##_msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

#define NT_ASSERTMSGW(_msg, _exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", _msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

#define NT_VERIFY     NT_ASSERT
#define NT_VERIFYMSG  NT_ASSERTMSG
#define NT_VERIFYMSGW NT_ASSERTMSGW

#endif // #if _MSC_VER >= 1300

#define RTL_VERIFY         ASSERT
#define RTL_VERIFYMSG      ASSERTMSG

#define RTL_SOFT_VERIFY    RTL_SOFT_ASSERT
#define RTL_SOFT_VERIFYMSG RTL_SOFT_ASSERTMSG

#else
#define ASSERT( exp )         ((void) 0)
#define ASSERTMSG( msg, exp ) ((void) 0)

#if _MSC_VER >= 1300

#define NT_ASSERT(_exp)           ((void) 0)
#define NT_ASSERTMSG(_msg, _exp)  ((void) 0)
#define NT_ASSERTMSGW(_msg, _exp) ((void) 0)

#define NT_VERIFY(_exp)           ((_exp) ? TRUE : FALSE)
#define NT_VERIFYMSG(_msg, _exp ) ((_exp) ? TRUE : FALSE)
#define NT_VERIFYMSGW(_msg, _exp) ((_exp) ? TRUE : FALSE)

#endif // #if _MSC_VER >= 1300

#define RTL_SOFT_ASSERT(_exp)          ((void) 0)
#define RTL_SOFT_ASSERTMSG(_msg, _exp) ((void) 0)

#define RTL_VERIFY( exp )         ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG( msg, exp ) ((exp) ? TRUE : FALSE)

#define RTL_SOFT_VERIFY(_exp)         ((_exp) ? TRUE : FALSE)
#define RTL_SOFT_VERIFYMSG(msg, _exp) ((_exp) ? TRUE : FALSE)

#endif // DBG

// end_ntddk end_wdm end_nthal end_ntifs end_ntndis
 
// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis
//
//  Doubly-linked list manipulation routines.
//


//
//  VOID
//  InitializeListHead32(
//      PLIST_ENTRY32 ListHead
//      );
//

#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)


VOID
FORCEINLINE
InitializeListHead(
    IN PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))



BOOLEAN
FORCEINLINE
RemoveEntryList(
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

PLIST_ENTRY
FORCEINLINE
RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}



PLIST_ENTRY
FORCEINLINE
RemoveTailList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}


VOID
FORCEINLINE
InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}


VOID
FORCEINLINE
InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}


//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

#endif // !MIDL_PASS

// end_wdm end_nthal end_ntifs end_ntndis


// end_ntddk

 
// begin_ntifs

//
// This enumerated type is used as the function return value of the function
// that is used to search the tree for a key. FoundNode indicates that the
// function found the key. Insert as left indicates that the key was not found
// and the node should be inserted as the left child of the parent. Insert as
// right indicates that the key was not found and the node should be inserted
//  as the right child of the parent.
//
typedef enum _TABLE_SEARCH_RESULT{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

//
//  The results of a compare can be less than, equal, or greater than.
//

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

//
//  Define the Avl version of the generic table package.  Note a generic table
//  should really be an opaque type.  We provide routines to manipulate the structure.
//
//  A generic table is package for inserting, deleting, and looking up elements
//  in a table (e.g., in a symbol table).  To use this package the user
//  defines the structure of the elements stored in the table, provides a
//  comparison function, a memory allocation function, and a memory
//  deallocation function.
//
//  Note: the user compare function must impose a complete ordering among
//  all of the elements, and the table does not allow for duplicate entries.
//

//
// Add an empty typedef so that functions can reference the
// a pointer to the generic table struct before it is declared.
//

struct _RTL_AVL_TABLE;

//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.
//

typedef
RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_AVL_COMPARE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    );

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
PVOID
(NTAPI *PRTL_AVL_ALLOCATE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    CLONG ByteSize
    );

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
VOID
(NTAPI *PRTL_AVL_FREE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    PVOID Buffer
    );

//
//  The match function takes as input the user data to be matched and a pointer
//  to some match data, which was passed along with the function pointer.  It
//  returns TRUE for a match and FALSE for no match.
//
//  RTL_AVL_MATCH_FUNCTION returns
//      STATUS_SUCCESS if the IndexRow matches
//      STATUS_NO_MATCH if the IndexRow does not match, but the enumeration should
//          continue
//      STATUS_NO_MORE_MATCHES if the IndexRow does not match, and the enumeration
//          should terminate
//


typedef
NTSTATUS
(NTAPI *PRTL_AVL_MATCH_FUNCTION) (
    struct _RTL_AVL_TABLE *Table,
    PVOID UserData,
    PVOID MatchData
    );

//
//  Define the balanced tree links and Balance field.  (No Rank field
//  defined at this time.)
//
//  Callers should treat this structure as opaque!
//
//  The root of a balanced binary tree is not a real node in the tree
//  but rather points to a real node which is the root.  It is always
//  in the table below, and its fields are used as follows:
//
//      Parent      Pointer to self, to allow for detection of the root.
//      LeftChild   NULL
//      RightChild  Pointer to real root
//      Balance     Undefined, however it is set to a convenient value
//                  (depending on the algorithm) prior to rebalancing
//                  in insert and delete routines.
//

typedef struct _RTL_BALANCED_LINKS {
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS;
typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;

//
//  To use the generic table package the user declares a variable of type
//  GENERIC_TABLE and then uses the routines described below to initialize
//  the table and to manipulate the table.  Note that the generic table
//  should really be an opaque type.
//

typedef struct _RTL_AVL_TABLE {
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE;
typedef RTL_AVL_TABLE *PRTL_AVL_TABLE;

//
//  The procedure InitializeGenericTable takes as input an uninitialized
//  generic table variable and pointers to the three user supplied routines.
//  This must be called for every individual generic table variable before
//  it can be used.
//

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl (
    PRTL_AVL_TABLE Table,
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_AVL_FREE_ROUTINE FreeRoutine,
    PVOID TableContext
    );

//
//  The function InsertElementGenericTable will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes AVL links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer,
    CLONG BufferSize,
    PBOOLEAN NewElement OPTIONAL
    );

//
//  The function InsertElementGenericTableFull will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes AVL links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//  This routine is passed the NodeOrParent and SearchResult from a
//  previous RtlLookupElementGenericTableFull.
//

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer,
    CLONG BufferSize,
    PBOOLEAN NewElement OPTIONAL,
    PVOID NodeOrParent,
    TABLE_SEARCH_RESULT SearchResult
    );

//
//  The function DeleteElementGenericTable will find and delete an element
//  from a generic table.  If the element is located and deleted the return
//  value is TRUE, otherwise if the element is not located the return value
//  is FALSE.  The user supplied input buffer is only used as a key in
//  locating the element in the table.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer
    );

//
//  The function LookupElementGenericTable will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element, otherwise if
//  the element is not located the return value is NULL.  The user supplied
//  input buffer is only used as a key in locating the element in the table.
//

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer
    );

//
//  The function LookupElementGenericTableFull will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element.  If the element is not
//  located then a pointer to the parent for the insert location is returned.  The
//  user must look at the SearchResult value to determine which is being returned.
//  The user can use the SearchResult and parent for a subsequent FullInsertElement
//  call to optimize the insert.
//

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer,
    OUT PVOID *NodeOrParent,
    OUT TABLE_SEARCH_RESULT *SearchResult
    );

//
//  The function EnumerateGenericTable will return to the caller one-by-one
//  the elements of of a table.  The return value is a pointer to the user
//  defined structure associated with the element.  The input parameter
//  Restart indicates if the enumeration should start from the beginning
//  or should return the next element.  If the are no more new elements to
//  return the return value is NULL.  As an example of its use, to enumerate
//  all of the elements in a table the user would write:
//
//      for (ptr = EnumerateGenericTable(Table, TRUE);
//           ptr != NULL;
//           ptr = EnumerateGenericTable(Table, FALSE)) {
//              :
//      }
//
//  NOTE:   This routine does not modify the structure of the tree, but saves
//          the last node returned in the generic table itself, and for this
//          reason requires exclusive access to the table for the duration of
//          the enumeration.
//

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl (
    PRTL_AVL_TABLE Table,
    BOOLEAN Restart
    );

//
//  The function EnumerateGenericTableWithoutSplaying will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
//           ptr != NULL;
//           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//  NOTE:   This routine does not modify either the structure of the tree
//          or the generic table itself, but must ensure that no deletes
//          occur for the duration of the enumeration, typically by having
//          at least shared access to the table for the duration.
//

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl (
    PRTL_AVL_TABLE Table,
    PVOID *RestartKey
    );

//
//  The function EnumerateGenericTableLikeADirectory will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...);
//           ptr != NULL;
//           ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//  NOTE:   This routine does not modify either the structure of the tree
//          or the generic table itself.  The table must only be acquired
//          shared for the duration of this call, and all synchronization
//          may optionally be dropped between calls.  Enumeration is always
//          correctly resumed in the most efficient manner possible via the
//          IN OUT parameters provided.
//
//  ******  Explain NextFlag.  Directory enumeration resumes from a key
//          requires more thought.  Also need the match pattern and IgnoreCase.
//          Should some structure be introduced to carry it all?
//

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_AVL_MATCH_FUNCTION MatchFunction,
    IN PVOID MatchData,
    IN ULONG NextFlag,
    IN OUT PVOID *RestartKey,
    IN OUT PULONG DeleteCount,
    IN OUT PVOID Buffer
    );

//
// The function GetElementGenericTable will return the i'th element
// inserted in the generic table.  I = 0 implies the first element,
// I = (RtlNumberGenericTableElements(Table)-1) will return the last element
// inserted into the generic table.  The type of I is ULONG.  Values
// of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
// an arbitrary element is deleted from the generic table it will cause
// all elements inserted after the deleted element to "move up".

NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl (
    PRTL_AVL_TABLE Table,
    ULONG I
    );

//
// The function NumberGenericTableElements returns a ULONG value
// which is the number of generic table elements currently inserted
// in the generic table.

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl (
    PRTL_AVL_TABLE Table
    );

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the input table is empty (i.e., does not contain any elements) and
//  FALSE otherwise.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl (
    PRTL_AVL_TABLE Table
    );

//
//  As an aid to allowing existing generic table users to do (in most
//  cases) a single-line edit to switch over to Avl table use, we
//  have the following defines and inline routine definitions which
//  redirect calls and types.  Note that the type override (performed
//  by #define below) will not work in the unexpected event that someone
//  has used a pointer or type specifier in their own #define, since
//  #define processing is one pass and does not nest.  The __inline
//  declarations below do not have this limitation, however.
//
//  To switch to using Avl tables, add the following line before your
//  includes:
//
//  #define RTL_USE_AVL_TABLES 0
//

#ifdef RTL_USE_AVL_TABLES

#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define PRTL_GENERIC_COMPARE_ROUTINE PRTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE PRTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE PRTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_TABLE RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE

#define RtlInitializeGenericTable               RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable            RtlInsertElementGenericTableAvl
#define RtlInsertElementGenericTableFull        RtlInsertElementGenericTableFullAvl
#define RtlDeleteElementGenericTable            RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable            RtlLookupElementGenericTableAvl
#define RtlLookupElementGenericTableFull        RtlLookupElementGenericTableFullAvl
#define RtlEnumerateGenericTable                RtlEnumerateGenericTableAvl
#define RtlEnumerateGenericTableWithoutSplaying RtlEnumerateGenericTableWithoutSplayingAvl
#define RtlGetElementGenericTable               RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements           RtlNumberGenericTableElementsAvl
#define RtlIsGenericTableEmpty                  RtlIsGenericTableEmptyAvl

#endif // RTL_USE_AVL_TABLES

 
//
//  Define the splay links and the associated manipulation macros and
//  routines.  Note that the splay_links should be an opaque type.
//  Routine are provided to traverse and manipulate the structure.
//

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;

//
//  The macro procedure InitializeSplayLinks takes as input a pointer to
//  splay link and initializes its substructure.  All splay link nodes must
//  be initialized before they are used in the different splay routines and
//  macros.
//
//  VOID
//  RtlInitializeSplayLinks (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlInitializeSplayLinks(Links) {    \
    PRTL_SPLAY_LINKS _SplayLinks;            \
    _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
    _SplayLinks->Parent = _SplayLinks;   \
    _SplayLinks->LeftChild = NULL;       \
    _SplayLinks->RightChild = NULL;      \
    }

//
//  The macro function Parent takes as input a pointer to a splay link in a
//  tree and returns a pointer to the splay link of the parent of the input
//  node.  If the input node is the root of the tree the return value is
//  equal to the input value.
//
//  PRTL_SPLAY_LINKS
//  RtlParent (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlParent(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->Parent \
    )

//
//  The macro function LeftChild takes as input a pointer to a splay link in
//  a tree and returns a pointer to the splay link of the left child of the
//  input node.  If the left child does not exist, the return value is NULL.
//
//  PRTL_SPLAY_LINKS
//  RtlLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlLeftChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->LeftChild \
    )

//
//  The macro function RightChild takes as input a pointer to a splay link
//  in a tree and returns a pointer to the splay link of the right child of
//  the input node.  If the right child does not exist, the return value is
//  NULL.
//
//  PRTL_SPLAY_LINKS
//  RtlRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlRightChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->RightChild \
    )

//
//  The macro function IsRoot takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the root of the tree,
//  otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsRoot (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsRoot(Links) (                          \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsLeftChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the left child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsLeftChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsLeftChild(Links) (                                   \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro function IsRightChild takes as input a pointer to a splay link
//  in a tree and returns TRUE if the input node is the right child of its
//  parent, otherwise it returns FALSE.
//
//  BOOLEAN
//  RtlIsRightChild (
//      PRTL_SPLAY_LINKS Links
//      );
//

#define RtlIsRightChild(Links) (                                   \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

//
//  The macro procedure InsertAsLeftChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the left child of the first node.  The first node must not
//  already have a left child, and the second node must not already have a
//  parent.
//
//  VOID
//  RtlInsertAsLeftChild (
//      PRTL_SPLAY_LINKS ParentLinks,
//      PRTL_SPLAY_LINKS ChildLinks
//      );
//

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks) { \
    PRTL_SPLAY_LINKS _SplayParent;                      \
    PRTL_SPLAY_LINKS _SplayChild;                       \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks);     \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);       \
    _SplayParent->LeftChild = _SplayChild;          \
    _SplayChild->Parent = _SplayParent;             \
    }

//
//  The macro procedure InsertAsRightChild takes as input a pointer to a splay
//  link in a tree and a pointer to a node not in a tree.  It inserts the
//  second node as the right child of the first node.  The first node must not
//  already have a right child, and the second node must not already have a
//  parent.
//
//  VOID
//  RtlInsertAsRightChild (
//      PRTL_SPLAY_LINKS ParentLinks,
//      PRTL_SPLAY_LINKS ChildLinks
//      );
//

#define RtlInsertAsRightChild(ParentLinks,ChildLinks) { \
    PRTL_SPLAY_LINKS _SplayParent;                       \
    PRTL_SPLAY_LINKS _SplayChild;                        \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks);      \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);        \
    _SplayParent->RightChild = _SplayChild;          \
    _SplayChild->Parent = _SplayParent;              \
    }

//
//  The Splay function takes as input a pointer to a splay link in a tree
//  and splays the tree.  Its function return value is a pointer to the
//  root of the splayed tree.
//

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay (
    PRTL_SPLAY_LINKS Links
    );

//
//  The Delete function takes as input a pointer to a splay link in a tree
//  and deletes that node from the tree.  Its function return value is a
//  pointer to the root of the tree.  If the tree is now empty, the return
//  value is NULL.
//

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete (
    PRTL_SPLAY_LINKS Links
    );

//
//  The DeleteNoSplay function takes as input a pointer to a splay link in a tree,
//  the caller's pointer to the root of the tree and deletes that node from the
//  tree.  Upon return the caller's pointer to the root node will correctly point
//  at the root of the tree.
//
//  It operationally differs from RtlDelete only in that it will not splay the tree.
//

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay (
    PRTL_SPLAY_LINKS Links,
    PRTL_SPLAY_LINKS *Root
    );

//
//  The SubtreeSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node of
//  the subtree rooted at the input node.  If there is not a successor, the
//  return value is NULL.
//

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor (
    PRTL_SPLAY_LINKS Links
    );

//
//  The SubtreePredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node of
//  the subtree rooted at the input node.  If there is not a predecessor,
//  the return value is NULL.
//

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor (
    PRTL_SPLAY_LINKS Links
    );

//
//  The RealSuccessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the successor of the input node within
//  the entire tree.  If there is not a successor, the return value is NULL.
//

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor (
    PRTL_SPLAY_LINKS Links
    );

//
//  The RealPredecessor function takes as input a pointer to a splay link
//  in a tree and returns a pointer to the predecessor of the input node
//  within the entire tree.  If there is not a predecessor, the return value
//  is NULL.
//

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor (
    PRTL_SPLAY_LINKS Links
    );

 
//
//  Define the generic table package.  Note a generic table should really
//  be an opaque type.  We provide routines to manipulate the structure.
//
//  A generic table is package for inserting, deleting, and looking up elements
//  in a table (e.g., in a symbol table).  To use this package the user
//  defines the structure of the elements stored in the table, provides a
//  comparison function, a memory allocation function, and a memory
//  deallocation function.
//
//  Note: the user compare function must impose a complete ordering among
//  all of the elements, and the table does not allow for duplicate entries.
//

//
//  Do not do the following defines if using Avl
//

#ifndef RTL_USE_AVL_TABLES

//
// Add an empty typedef so that functions can reference the
// a pointer to the generic table struct before it is declared.
//

struct _RTL_GENERIC_TABLE;

//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.
//

typedef
RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_GENERIC_COMPARE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    );

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
PVOID
(NTAPI *PRTL_GENERIC_ALLOCATE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    CLONG ByteSize
    );

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
VOID
(NTAPI *PRTL_GENERIC_FREE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    PVOID Buffer
    );

//
//  To use the generic table package the user declares a variable of type
//  GENERIC_TABLE and then uses the routines described below to initialize
//  the table and to manipulate the table.  Note that the generic table
//  should really be an opaque type.
//

typedef struct _RTL_GENERIC_TABLE {
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

//
//  The procedure InitializeGenericTable takes as input an uninitialized
//  generic table variable and pointers to the three user supplied routines.
//  This must be called for every individual generic table variable before
//  it can be used.
//

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable (
    PRTL_GENERIC_TABLE Table,
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    PVOID TableContext
    );

//
//  The function InsertElementGenericTable will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes splay links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer,
    CLONG BufferSize,
    PBOOLEAN NewElement OPTIONAL
    );

//
//  The function InsertElementGenericTableFull will insert a new element
//  in a table.  It does this by allocating space for the new element
//  (this includes splay links), inserting the element in the table, and
//  then returning to the user a pointer to the new element.  If an element
//  with the same key already exists in the table the return value is a pointer
//  to the old element.  The optional output parameter NewElement is used
//  to indicate if the element previously existed in the table.  Note: the user
//  supplied Buffer is only used for searching the table, upon insertion its
//  contents are copied to the newly created element.  This means that
//  pointer to the input buffer will not point to the new element.
//  This routine is passed the NodeOrParent and SearchResult from a
//  previous RtlLookupElementGenericTableFull.
//

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer,
    CLONG BufferSize,
    PBOOLEAN NewElement OPTIONAL,
    PVOID NodeOrParent,
    TABLE_SEARCH_RESULT SearchResult
    );

//
//  The function DeleteElementGenericTable will find and delete an element
//  from a generic table.  If the element is located and deleted the return
//  value is TRUE, otherwise if the element is not located the return value
//  is FALSE.  The user supplied input buffer is only used as a key in
//  locating the element in the table.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
    );

//
//  The function LookupElementGenericTable will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element, otherwise if
//  the element is not located the return value is NULL.  The user supplied
//  input buffer is only used as a key in locating the element in the table.
//

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
    );

//
//  The function LookupElementGenericTableFull will find an element in a generic
//  table.  If the element is located the return value is a pointer to
//  the user defined structure associated with the element.  If the element is not
//  located then a pointer to the parent for the insert location is returned.  The
//  user must look at the SearchResult value to determine which is being returned.
//  The user can use the SearchResult and parent for a subsequent FullInsertElement
//  call to optimize the insert.
//

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer,
    OUT PVOID *NodeOrParent,
    OUT TABLE_SEARCH_RESULT *SearchResult
    );

//
//  The function EnumerateGenericTable will return to the caller one-by-one
//  the elements of of a table.  The return value is a pointer to the user
//  defined structure associated with the element.  The input parameter
//  Restart indicates if the enumeration should start from the beginning
//  or should return the next element.  If the are no more new elements to
//  return the return value is NULL.  As an example of its use, to enumerate
//  all of the elements in a table the user would write:
//
//      for (ptr = EnumerateGenericTable(Table, TRUE);
//           ptr != NULL;
//           ptr = EnumerateGenericTable(Table, FALSE)) {
//              :
//      }
//
//
//  PLEASE NOTE:
//
//      If you enumerate a GenericTable using RtlEnumerateGenericTable, you
//      will flatten the table, turning it into a sorted linked list.
//      To enumerate the table without perturbing the splay links, use
//      RtlEnumerateGenericTableWithoutSplaying

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable (
    PRTL_GENERIC_TABLE Table,
    BOOLEAN Restart
    );

//
//  The function EnumerateGenericTableWithoutSplaying will return to the
//  caller one-by-one the elements of of a table.  The return value is a
//  pointer to the user defined structure associated with the element.
//  The input parameter RestartKey indicates if the enumeration should
//  start from the beginning or should return the next element.  If the
//  are no more new elements to return the return value is NULL.  As an
//  example of its use, to enumerate all of the elements in a table the
//  user would write:
//
//      RestartKey = NULL;
//      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
//           ptr != NULL;
//           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
//              :
//      }
//
//  If RestartKey is NULL, the package will start from the least entry in the
//  table, otherwise it will start from the last entry returned.
//
//
//  Note that unlike RtlEnumerateGenericTable, this routine will NOT perturb
//  the splay order of the tree.
//

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying (
    PRTL_GENERIC_TABLE Table,
    PVOID *RestartKey
    );

//
// The function GetElementGenericTable will return the i'th element
// inserted in the generic table.  I = 0 implies the first element,
// I = (RtlNumberGenericTableElements(Table)-1) will return the last element
// inserted into the generic table.  The type of I is ULONG.  Values
// of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
// an arbitrary element is deleted from the generic table it will cause
// all elements inserted after the deleted element to "move up".

NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
    PRTL_GENERIC_TABLE Table,
    ULONG I
    );

//
// The function NumberGenericTableElements returns a ULONG value
// which is the number of generic table elements currently inserted
// in the generic table.

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
    PRTL_GENERIC_TABLE Table
    );

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the input table is empty (i.e., does not contain any elements) and
//  FALSE otherwise.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty (
    PRTL_GENERIC_TABLE Table
    );

#endif // RTL_USE_AVL_TABLES

// end_ntifs
 
//
//  Heap Allocator
//

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeHeapManager(
    VOID
    );

// begin_ntifs

typedef NTSTATUS
(NTAPI * PRTL_HEAP_COMMIT_ROUTINE)(
    IN PVOID Base,
    IN OUT PVOID *CommitAddress,
    IN OUT PSIZE_T CommitSize
    );

typedef struct _RTL_HEAP_PARAMETERS {
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[ 2 ];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
    IN ULONG Flags,
    IN PVOID HeapBase OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    );

#define HEAP_NO_SERIALIZE               0x00000001      // winnt
#define HEAP_GROWABLE                   0x00000002      // winnt
#define HEAP_GENERATE_EXCEPTIONS        0x00000004      // winnt
#define HEAP_ZERO_MEMORY                0x00000008      // winnt
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010      // winnt
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020      // winnt
#define HEAP_FREE_CHECKING_ENABLED      0x00000040      // winnt
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080      // winnt

#define HEAP_CREATE_ALIGN_16            0x00010000      // winnt Create heap with 16 byte alignment (obsolete)
#define HEAP_CREATE_ENABLE_TRACING      0x00020000      // winnt Create heap call tracing enabled (obsolete)
#define HEAP_CREATE_ENABLE_EXECUTE      0x00040000      // winnt Create heap with executable pages

#define HEAP_SETTABLE_USER_VALUE        0x00000100
#define HEAP_SETTABLE_USER_FLAG1        0x00000200
#define HEAP_SETTABLE_USER_FLAG2        0x00000400
#define HEAP_SETTABLE_USER_FLAG3        0x00000800
#define HEAP_SETTABLE_USER_FLAGS        0x00000E00

#define HEAP_CLASS_0                    0x00000000      // process heap
#define HEAP_CLASS_1                    0x00001000      // private heap
#define HEAP_CLASS_2                    0x00002000      // Kernel Heap
#define HEAP_CLASS_3                    0x00003000      // GDI heap
#define HEAP_CLASS_4                    0x00004000      // User heap
#define HEAP_CLASS_5                    0x00005000      // Console heap
#define HEAP_CLASS_6                    0x00006000      // User Desktop heap
#define HEAP_CLASS_7                    0x00007000      // Csrss Shared heap
#define HEAP_CLASS_8                    0x00008000      // Csr Port heap
#define HEAP_CLASS_MASK                 0x0000F000

#define HEAP_MAXIMUM_TAG                0x0FFF              // winnt
#define HEAP_GLOBAL_TAG                 0x0800
#define HEAP_PSEUDO_TAG_FLAG            0x8000              // winnt
#define HEAP_TAG_SHIFT                  18                  // winnt
#define HEAP_MAKE_TAG_FLAGS( b, o ) ((ULONG)((b) + ((o) << 18)))  // winnt
#define HEAP_TAG_MASK                  (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

#define HEAP_CREATE_VALID_MASK         (HEAP_NO_SERIALIZE |             \
                                        HEAP_GROWABLE |                 \
                                        HEAP_GENERATE_EXCEPTIONS |      \
                                        HEAP_ZERO_MEMORY |              \
                                        HEAP_REALLOC_IN_PLACE_ONLY |    \
                                        HEAP_TAIL_CHECKING_ENABLED |    \
                                        HEAP_FREE_CHECKING_ENABLED |    \
                                        HEAP_DISABLE_COALESCE_ON_FREE | \
                                        HEAP_CLASS_MASK |               \
                                        HEAP_CREATE_ALIGN_16 |          \
                                        HEAP_CREATE_ENABLE_TRACING |    \
                                        HEAP_CREATE_ENABLE_EXECUTE)

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
    IN PVOID HeapHandle
    );

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

// end_ntifs

NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlZeroHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSYSAPI
VOID
NTAPI
RtlProtectHeap(
    IN PVOID HeapHandle,
    IN BOOLEAN MakeReadOnly
    );

//
// See NTURTL.H for remaining, user mode only heap functions.
//

//
// The types PACQUIRE_LOCK_ROUTINE and PRELEASE_LOCK_ROUTINE are prototypes
// for routines to acquire and release locks in kernel and user mode.
//

typedef
NTSTATUS
(NTAPI *PRTL_INITIALIZE_LOCK_ROUTINE) (
    PVOID Lock
    );

typedef
NTSTATUS
(NTAPI *PRTL_ACQUIRE_LOCK_ROUTINE) (
    PVOID Lock
    );

typedef
NTSTATUS
(NTAPI *PRTL_RELEASE_LOCK_ROUTINE) (
    PVOID Lock
    );

typedef
NTSTATUS
(NTAPI *PRTL_DELETE_LOCK_ROUTINE) (
    PVOID Lock
    );

typedef
BOOLEAN
(NTAPI *PRTL_OKAY_TO_LOCK_ROUTINE) (
    IN PVOID Lock
    );

NTSYSAPI
ULONG
NTAPI
RtlGetNtGlobalFlags(
    VOID
    );

 
//
//  Functions to capture a stack back trace
//
// begin_ntddk begin_nthal begin_ntifs begin_ntndis

#if defined (_MSC_VER) && ( _MSC_VER >= 900 )

PVOID
_ReturnAddress (
    VOID
    );

#pragma intrinsic(_ReturnAddress)

#endif

#if (defined(_M_AMD64) && !defined(_REALLY_GET_CALLERS_CALLER_))

#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;

#else

NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
    );

#endif

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    );

// end_ntddk end_nthal end_ntifs end_ntndis

NTSYSAPI
USHORT
NTAPI
RtlLogStackBackTrace(
    VOID
    );

// begin_winnt

NTSYSAPI
VOID
NTAPI
RtlCaptureContext (
    OUT PCONTEXT ContextRecord
    );

// end_winnt

NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash OPTIONAL
   );

#define MAX_STACK_DEPTH 32

typedef struct _RTL_PROCESS_BACKTRACE_INFORMATION {
    PCHAR SymbolicBackTrace;        // Not filled in
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[ MAX_STACK_DEPTH ];
} RTL_PROCESS_BACKTRACE_INFORMATION, *PRTL_PROCESS_BACKTRACE_INFORMATION;

typedef struct _RTL_PROCESS_BACKTRACES {
    ULONG CommittedMemory;
    ULONG ReservedMemory;
    ULONG NumberOfBackTraceLookups;
    ULONG NumberOfBackTraces;
    RTL_PROCESS_BACKTRACE_INFORMATION BackTraces[ 1 ];
} RTL_PROCESS_BACKTRACES, *PRTL_PROCESS_BACKTRACES;

//
// Capture stack context
//

typedef struct _RTL_STACK_CONTEXT_ENTRY {

    ULONG_PTR Address; // stack address
    ULONG_PTR Data;    // stack contents

} RTL_STACK_CONTEXT_ENTRY, * PRTL_STACK_CONTEXT_ENTRY;

typedef struct _RTL_STACK_CONTEXT {

    ULONG NumberOfEntries;
    RTL_STACK_CONTEXT_ENTRY Entry[1];

} RTL_STACK_CONTEXT, * PRTL_STACK_CONTEXT;

NTSYSAPI
ULONG
NTAPI
RtlCaptureStackContext (
    OUT PULONG_PTR Callers,
    OUT PRTL_STACK_CONTEXT Context,
    IN ULONG Limit
    );

//
// Trace database support (User/Kernel mode).
//

#define RTL_TRACE_IN_USER_MODE       0x00000001
#define RTL_TRACE_IN_KERNEL_MODE     0x00000002
#define RTL_TRACE_USE_NONPAGED_POOL  0x00000004
#define RTL_TRACE_USE_PAGED_POOL     0x00000008

//
// RTL_TRACE_BLOCK
//

typedef struct _RTL_TRACE_BLOCK {

    ULONG Magic;
    ULONG Count;
    ULONG Size;

    SIZE_T UserCount;
    SIZE_T UserSize;
    PVOID UserContext;

    struct _RTL_TRACE_BLOCK * Next;
    PVOID * Trace;

} RTL_TRACE_BLOCK, * PRTL_TRACE_BLOCK;

//
// RTL_TRACE_HASH_FUNCTION
//

typedef ULONG (* RTL_TRACE_HASH_FUNCTION) (ULONG Count, PVOID * Trace);

//
// RTL_TRACE_DATABASE
//

typedef struct _RTL_TRACE_DATABASE * PRTL_TRACE_DATABASE;

//
// RTL_TRACE_ENUMERATE
//

typedef struct _RTL_TRACE_ENUMERATE {

    PRTL_TRACE_DATABASE Database;
    ULONG Index;
    PRTL_TRACE_BLOCK Block;

} RTL_TRACE_ENUMERATE, * PRTL_TRACE_ENUMERATE;

//
// Trace database interfaces
//

PRTL_TRACE_DATABASE
RtlTraceDatabaseCreate (
    IN ULONG Buckets,
    IN SIZE_T MaximumSize OPTIONAL,
    IN ULONG Flags, // OPTIONAL in User mode
    IN ULONG Tag,   // OPTIONAL in User mode
    IN RTL_TRACE_HASH_FUNCTION HashFunction OPTIONAL
    );

BOOLEAN
RtlTraceDatabaseDestroy (
    IN PRTL_TRACE_DATABASE Database
    );

BOOLEAN
RtlTraceDatabaseValidate (
    IN PRTL_TRACE_DATABASE Database
    );

BOOLEAN
RtlTraceDatabaseAdd (
    IN PRTL_TRACE_DATABASE Database,
    IN ULONG Count,
    IN PVOID * Trace,
    OUT PRTL_TRACE_BLOCK * TraceBlock OPTIONAL
    );

BOOLEAN
RtlTraceDatabaseFind (
    PRTL_TRACE_DATABASE Database,
    IN ULONG Count,
    IN PVOID * Trace,
    OUT PRTL_TRACE_BLOCK * TraceBlock OPTIONAL
    );

BOOLEAN
RtlTraceDatabaseEnumerate (
    PRTL_TRACE_DATABASE Database,
    OUT PRTL_TRACE_ENUMERATE Enumerate,
    OUT PRTL_TRACE_BLOCK * TraceBlock
    );

VOID
RtlTraceDatabaseLock (
    IN PRTL_TRACE_DATABASE Database
    );

VOID
RtlTraceDatabaseUnlock (
    IN PRTL_TRACE_DATABASE Database
    );


VOID
RtlpGetStackLimits (
    OUT PULONG_PTR LowLimit,
    OUT PULONG_PTR HighLimit
    );

 
//
// Subroutines for dealing with Win32 ATOMs.  Used by kernel mode window
// manager and user mode implementation of Win32 ATOM API calls in KERNEL32
//

#define RTL_ATOM_MAXIMUM_INTEGER_ATOM   (RTL_ATOM)0xC000
#define RTL_ATOM_INVALID_ATOM           (RTL_ATOM)0x0000
#define RTL_ATOM_TABLE_DEFAULT_NUMBER_OF_BUCKETS 37
#define RTL_ATOM_MAXIMUM_NAME_LENGTH    255
#define RTL_ATOM_PINNED 0x01

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeAtomPackage(
    IN ULONG AllocationTag
    );

NTSYSAPI
NTSTATUS


