#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "util/index.h"

#include "util/types.h"
#include "util/parser.h"
#include "util/pmemory.h"
#include "util/uthash.h"

// Number of documents handled
static unsigned int documentsCountInCorpus = 0;

// Initial size of the posting list array
static const unsigned int kInitialPostingListSize = 10;

// By how much the size is multiplied when reallocating posting list
static const unsigned int kSizeAugmentFactor = 2;

// Returns the idf (inverse document frequency) of a term
static double inverseDocumentFrequency(const TermEntry* term);

// Returns the number of occurrences of a token in a document
static unsigned int termFrequency(const TermEntry* term,
                                  unsigned int documentId);

// Adds a new TermEntry (must alredy be allocated) to the vocabulary.
static Vocabulary* addNewTermEntry(Vocabulary* vocabulary, TermEntry* term,
                                   wchar_t* token, unsigned int docId);

//See util/index.h
int compareTermEntries(TermEntry* t1, TermEntry* t2){
  return wcscmp(t1->token, t2->token);
}

// See util/index.h
Vocabulary* tryToAddToken(Vocabulary* vocabulary, wchar_t* token,
                          unsigned int docId, bool* noMemory) {
  // Add one to count if this is a new document
  if (docId + 1 > documentsCountInCorpus) ++documentsCountInCorpus;

  TermEntry* term = NULL;

  HASH_FIND(hh, vocabulary, token,
            wcslen(token) * sizeof(wchar_t), term);
  if (!term) {
    // Not found: allocate TermEntry structure
    if (!initTerm(&term, token)) {
      pFreeTerm(term);
      term = NULL;
      *noMemory = true;
      return vocabulary;
    }

    *noMemory = false;
    // Populate term and add it to vocab
    vocabulary = addNewTermEntry(vocabulary, term, token, docId);
  } else {
    // Found and not duplicate: just adding the docId to the list
    *noMemory = addToTermEntry(1, term, docId);
  }

  return vocabulary;
}

// See util/index.h
Vocabulary* fpurgeIndex(FILE* output, Vocabulary* vocabulary) {
  //Sort the hash table to serialize a sorted index
  HASH_SORT(vocabulary, compareTermEntries);

  TermEntry* t = NULL;
  TermEntry* tmp = NULL;
  HASH_ITER(hh, vocabulary, t, tmp) {
    fprintTerm(output, t, SERIALIZATION);
    HASH_DEL(vocabulary, t);
    pFreeTerm(t);
    t = NULL;
  }

  return vocabulary;
}

// See util/index.h
TermEntry* readTermEntry(FILE* input) {
  if (!input) return NULL;  // Nothing to do

  wchar_t buffer[kBufferSize];
  memset(buffer, 0, kBufferSize * sizeof(wchar_t));  // Set to empty string

  if (!fgetws(buffer, kBufferSize, input)) return NULL;  // Nothing to read

  wchar_t readStr[kBufferSize];
  memset(readStr, 0, kBufferSize * sizeof(wchar_t));  // Set to empty string
  if (!nextToken(buffer, input, readStr)) return NULL;  // Parse error

  TermEntry* term;
  if (!initTerm(&term, readStr)) {
    pFreeTerm(term);
    term = NULL;
    return NULL;
  }
  term->listLength = 0;
  term->listSize = kInitialPostingListSize;

  for (unsigned int i = 0; !endOfBuffer(buffer); ++i) {
    if (!nextToken(buffer, input, readStr)) {
      pFreeTerm(term);
      term = NULL;
      return NULL;  // Parse Error
    }
    unsigned int docId = (unsigned int) wcstoul(readStr, NULL, 10);

    if (!nextToken(buffer, input, readStr)) {
      pFreeTerm(term);
      term = NULL;
      return NULL;  // Parse Error
    }

    unsigned int occurrences = (unsigned int) wcstoul(readStr, NULL, 10);
    addToTermEntry(occurrences, term, docId);
  }

  return term;
}

// See util/index.h
void fprintTerm(FILE* output, const TermEntry* t, TermPrintMode printMode) {
  switch (printMode) {
    case TEST_TFIDF:
      fprintf(output, "%ls: ", t->token);
      fprintf(output, "idf=%.2f, in docs: %u(tf = %d)",
              inverseDocumentFrequency(t),
              t->postingList[0].docId,
              termFrequency(t, t->postingList[0].docId));
      for (unsigned int i = 1; i < t->listLength; ++i) {
        fprintf(output, ", %u(tf = %d)",
                t->postingList[i].docId,
                termFrequency(t, t->postingList[i].docId));
      }
      fprintf(output, "\n");
      break;

    case TEST_SIMPLE:
      fprintf(output, "%ls: ", t->token);
      fprintf(output, "%u", t->postingList[0].docId);
      for (unsigned int i = 1; i < t->listLength; ++i) {
        fprintf(output, ", %u", t->postingList[i].docId);
      }
      fprintf(output, "\n");
      break;

    case SERIALIZATION:
      fprintf(output, "%ls", t->token);
      for (unsigned int i = 0; i < t->listLength; ++i) {
        fprintf(output, " %u %u", t->postingList[i].docId,
                t->postingList[i].occurrences);
      }
      fprintf(output, "\n");
      break;
  }
}

// See util/index.h
void pFreeTerm(TermEntry* t) {
  if (!t) return;  // Nothing to do

  if (t->postingList) {
    pFree(t->postingList, t->listSize * sizeof(PostingListEntry));
    t->postingList = NULL;
  }

  if (t->token) {
    pFree(t->token, (wcslen(t->token) + 1) * sizeof(wchar_t));
    t->token = NULL;
  }

  pFree(t, sizeof(TermEntry));
  t = NULL;
}

// See top of this file
static double inverseDocumentFrequency(const TermEntry* term) {
  return log10((double) documentsCountInCorpus / term->listLength);
}

// See top of this file
static unsigned int termFrequency(const TermEntry* term,
                                  unsigned int documentId) {
  for (unsigned int i = 0; i < term->listLength; ++i) {
    if (term->postingList[i].docId == documentId) {
      return term->postingList[i].occurrences;
    }
  }
  return 0;  // Not in the document
}

// See util/index.h
bool addToTermEntry(int occurrence, TermEntry* term, unsigned int docId) {
  if (term->listLength >= term->listSize) {
    unsigned int previousSize = term->listSize;
    PostingListEntry* postingList = (PostingListEntry*)
        pRealloc(term->postingList, previousSize * sizeof(PostingListEntry),
                 previousSize * kSizeAugmentFactor * sizeof(PostingListEntry));
    if (!postingList) return true;  // No memory left
    term->postingList = postingList;
    term->listSize *= kSizeAugmentFactor;
  }

  unsigned int docIdFound;
  // If the document id is in the posting list, just increment occurrences
  for (docIdFound = 0; docIdFound < term->listLength; ++docIdFound) {
    if (term->postingList[docIdFound].docId == docId){
      term->postingList[docIdFound].occurrences += occurrence;
      break;
    }
  }

  // If no entry for this term in this document yet, add docId to the list
  if (docIdFound == term->listLength) {
    term->listLength++;
    term->postingList[term->listLength - 1].docId = docId;
    term->postingList[term->listLength - 1].occurrences = occurrence;
  }

  return false;
}

// See top of this file
static Vocabulary* addNewTermEntry(Vocabulary* vocabulary, TermEntry* term,
                                   wchar_t* token, unsigned int docId) {
  wcscpy(term->token, token);

  term->listLength = 1;
  term->listSize = kInitialPostingListSize;
  term->postingList[0].docId = docId;
  term->postingList[0].occurrences = 1;

  // Add it to the hash map
  HASH_ADD_KEYPTR(hh, vocabulary, term->token,
                  wcslen(term->token) * sizeof(wchar_t), term);

  return vocabulary;
}

// See top of this file
bool initTerm(TermEntry** pTerm, wchar_t* token) {
  *pTerm = (TermEntry*) pMalloc(sizeof(TermEntry));
  if (!*pTerm) return false;

  (*pTerm)->token = NULL;
  (*pTerm)->postingList = NULL;

  (*pTerm)->token = (wchar_t*) pMalloc(sizeof(wchar_t) * (wcslen(token) + 1));
  if (!(*pTerm)->token) return false;
  wcscpy((*pTerm)->token, token);

  (*pTerm)->postingList = (PostingListEntry*) pMalloc(sizeof(PostingListEntry)
                                                  * kInitialPostingListSize);
  if (!(*pTerm)->postingList) return false;
  memset((*pTerm)->postingList, 0, sizeof(PostingListEntry) * kInitialPostingListSize);

  (*pTerm)->listLength = 0;
  (*pTerm)->listSize = kInitialPostingListSize;

  return true;
}
