#ifndef CONSTRUCT_TOKEN_H_
#define CONSTRUCT_TOKEN_H_

#include <stdio.h>

#include "util/types.h"
#include "util/uthash.h"

// Posting list entry used in an array of PostingListEntries
typedef struct {
  unsigned int docId;
  unsigned int occurrences;  // Occurrences of the term in the document
} PostingListEntry;

// Term entry used in a hash table pointing to a PostingListEntry array
typedef struct {
  // Key: the token name
  wchar_t* token;

  // Payload: PostingListEntry array
  PostingListEntry* postingList;  // Posting list table
  unsigned int listLength;  // Number of filled PostingListEntry
  unsigned int listSize;  // Number of allocated PostingListEntry

  // Macro used in order to use this as a uthash hash table
  UT_hash_handle hh;
} TermEntry;

// How to print the term
typedef enum {
  TEST_SIMPLE,
  TEST_TFIDF,
  SERIALIZATION
} TermPrintMode;

// A TermEntry is a hash table itself (see uthash doc)
typedef TermEntry Vocabulary;

// Adds one to the occurrence count of a term for a docId.
// May resize postingList if the docId is not already present.
// Returns true if there is no memory left, false otherwise.
bool addToTermEntry(int occcurence, TermEntry* term, unsigned int docId);

// Compares the key of each TermEntry, allowing to sort them.
// Returns positive value if t1 > t2, negative value if t1 < t2, 0 if t1 == t2.
int compareTermEntries(TermEntry* t1, TermEntry* t2);

// Adds token occurrence for a docId in the index (vocab + posting list).
// Assumes docId is equal to or one more than the docId of last call.
// Sets noMemory to true if it was impossible to add the token, false otherwise.
// Returns the vocabulary once the token was added.
Vocabulary* tryToAddToken(Vocabulary* vocabulary, wchar_t* token,
                          unsigned int docId, bool* noMemory);

// Serializes the index (vocab + posting list) to output and frees its memory.
// The output file should be open in text and write mode.
// Returns the vocabulary once the index is purged.
Vocabulary* fpurgeIndex(FILE* output, Vocabulary* vocabulary);

// Reads a TermEntry from the given input and returns it (or NULL if error).
TermEntry* readTermEntry(FILE* input);

// Displays a token key and payload on the output.
// If printMode is set to TF_IDF, the frequencies are also displayed.
// If it is set to SERIALIZATION, the output is printed like is is meant to be
// serialized.
void fprintTerm(FILE* output, const TermEntry* t, TermPrintMode printMode);

// Frees the memory of a TermEntry and all it points to, including posting list.
// Does not call pFree for NULL pointers.
// CAUTION: does NOT remove the token pointer from Vocabulary, you MUST do it!
void pFreeTerm(TermEntry* t);

// Allocates memory for a new TermEntry, changing the pointed pointer.
// Sets the token to token.
// Returns false if there is not enough memory, true otherwise.
bool initTerm(TermEntry** pTerm, wchar_t* token);

#endif  // CONSTRUCT_TOKEN_H_
