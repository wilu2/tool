#include <algorithm>
#include <map>
#include "diff.h"

using std::string;
using std::vector;
using std::map;
using std::min;
using std::max;

namespace xwb {
  
const int Diff_EditCost = 4;

static XChar add_char;
static XChar del_char;


int diff_commonPrefix(vector<XChar>& char_list_x, int start_x, int end_x, 
                      vector<XChar>& char_list_y, int start_y, int end_y) 
{
    // Performance analysis: https://neil.fraser.name/news/2007/10/09/
    int n = min(end_x - start_x + 1, end_y - start_y+1);
    for (int i = 0; i < n; i++) 
    {
        if (char_list_x[start_x+i].text != char_list_y[start_y+i].text) 
        {
            return i;
        }
    }
    return n;
}


/**
 * Determine the common suffix of two strings.
 * @param text1 First string.
 * @param text2 Second string.
 * @return The number of characters common to the end of each string.
 */
int diff_commonSuffix(vector<XChar>& char_list_x, int start_x, int end_x, 
                      vector<XChar>& char_list_y, int start_y, int end_y) 
{
    // Performance analysis: https://neil.fraser.name/news/2007/10/09/
    int text1_length = end_x - start_x+1;
    int text2_length = end_y - start_y+1;
    int n = min(text1_length, text2_length);
    for (int i = 0; i < n; i++) 
    {
        if (char_list_x[end_x-i].text != char_list_y[end_y-i].text) 
        {
            return i;
        }
    }
    return n;
}

/**
 * Reorder and merge like edit sections.  Merge equalities.
 * Any edit section can move as long as it doesn't cross an equality.
 * @param diffs List of Diff objects.
 */

// void diff_cleanupMerge(vector<Diff>& diffs)
// {
//     // Add a dummy entry at the end.
//     diffs.emplace_back(EQUAL, string());


//     int pointer = 0;
//     int count_delete = 0;
//     int count_insert = 0;
//     string text_delete;
//     string text_insert;
//     int commonlength;

//     while (pointer < diffs.size())
//     {
//         switch (diffs[pointer].operation)
//         {
//         case INSERT:
//             count_insert++;
//             text_insert += diffs[pointer].text;
//             pointer++;
//             break;
//         case DELETE:
//             count_delete++;
//             text_delete += diffs[pointer].text;
//             pointer++;
//             break;
//         case EQUAL:
//             // Upon reaching an equality, check for prior redundancies.
//             if (count_delete + count_insert > 1)
//             {
//                 if (count_delete != 0 && count_insert != 0)
//                 {
//                     // Factor out any common prefixies.
//                     commonlength = diff_commonPrefix(text_insert, text_delete);
//                     if (commonlength != 0)
//                     {
//                         if ((pointer - count_delete - count_insert) > 0 &&
//                             diffs[pointer - count_delete - count_insert - 1].operation == EQUAL)
//                         {
//                             diffs[pointer - count_delete - count_insert - 1].text += text_insert.substr(0, commonlength);
//                         }
//                         else
//                         {
//                             diffs.insert(diffs.begin(), Diff(EQUAL, text_insert.substr(0, commonlength)));
//                             pointer++;
//                         }
//                         text_insert = text_insert.substr(commonlength);
//                         text_delete = text_delete.substr(commonlength);
//                     }
//                     // Factor out any common suffixies.
//                     commonlength = diff_commonSuffix(text_insert, text_delete);
//                     if (commonlength != 0)
//                     {
//                         diffs[pointer].text = text_insert.substr(text_insert.size() - commonlength) + diffs[pointer].text;
//                         text_insert = text_insert.substr(0, text_insert.size() - commonlength);
//                         text_delete = text_delete.substr(0, text_delete.size() - commonlength);
//                     }
//                 }
//                 // Delete the offending records and add the merged ones.
//                 pointer -= count_delete + count_insert;
//                 //diffs.Splice(pointer, count_delete + count_insert);
//                 diffs.erase(diffs.begin() + pointer, diffs.begin() + pointer+ count_delete + count_insert);

//                 if (text_delete.size() != 0)
//                 {
//                     diffs.insert(diffs.begin() + pointer, Diff(DELETE, text_delete));
//                     pointer++;
//                 }

//                 if (text_insert.size() != 0)
//                 {
//                     diffs.insert(diffs.begin() + pointer, Diff(INSERT, text_insert));
//                     pointer++;
//                 }
//                 pointer++;
//             }
//             else if (pointer != 0 && diffs[pointer - 1].operation == EQUAL)
//             {
//                 // Merge this equality with the previous one.
//                 diffs[pointer - 1].text += diffs[pointer].text;
//                 //diffs.RemoveAt(pointer);
//                 diffs.erase(diffs.begin() + pointer);
//             }
//             else
//             {
//                 pointer++;
//             }
//             count_insert = 0;
//             count_delete = 0;
//             text_delete.clear();
//             text_insert.clear();
//             break;
//         }
//     }


//     if (diffs[diffs.size() - 1].text.size() == 0)
//     {
//         diffs.erase(diffs.begin() + diffs.size() - 1); // Remove the dummy entry at the end.
//     }

//     // Second pass: look for single edits surrounded on both sides by
//     // equalities which can be shifted sideways to eliminate an equality.
//     // e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
//     bool changes = false;
//     pointer = 1;
//     // Intentionally ignore the first and last element (don't need checking).
//     while (pointer < (diffs.size() - 1))
//     {
//         if (diffs[pointer - 1].operation == EQUAL && diffs[pointer + 1].operation == EQUAL)
//         {
//             // This is a single edit surrounded by equalities.
//             if (diffs[pointer].text.find(diffs[pointer - 1].text) == (int)diffs[pointer].text.size() - (int)diffs[pointer-1].text.size())
//             {
//                 // Shift the edit over the previous equality.
//                 diffs[pointer].text = diffs[pointer - 1].text +
//                                       diffs[pointer].text.substr(0, diffs[pointer].text.size() -
//                                                                            diffs[pointer - 1].text.size());
//                 diffs[pointer + 1].text = diffs[pointer - 1].text + diffs[pointer + 1].text;
//                 //diffs.Splice(pointer - 1, 1);
//                 diffs.erase(diffs.begin() + pointer - 1);
//                 changes = true;
//             }
//             else if (diffs[pointer].text.find(diffs[pointer + 1].text) == 0)
//             {
//                 // Shift the edit over the next equality.
//                 diffs[pointer - 1].text += diffs[pointer + 1].text;
//                 diffs[pointer].text =
//                     diffs[pointer].text.substr(diffs[pointer + 1].text.size()) + diffs[pointer + 1].text;
//                 //diffs.Splice(pointer + 1, 1);
//                 diffs.erase(diffs.begin() + pointer+1);
//                 changes = true;
//             }
//         }
//         pointer++;
//     }
//     // If shifts were made, the diff needs reordering and another shift sweep.
//     if (changes)
//     {
//         diff_cleanupMerge(diffs);
//     }
// }


//     static class CompatibilityExtensions {
//     // JScript splice function
//     public static List<T> Splice<T>(this List<T> input, int start, int count,
//         params T[] objects) {
//       List<T> deletedRange = input.GetRange(start, count);
//       input.RemoveRange(start, count);
//       input.InsertRange(start, objects);

//       return deletedRange;
//     }

//     // Java substring function
//     public static string JavaSubstring(this string s, int begin, int end) {
//       return s.Substring(begin, end - begin);
//     }
//   }












    










    // /**
    //  * Determine if the suffix of one string is the prefix of another.
    //  * @param text1 First string.
    //  * @param text2 Second string.
    //  * @return The number of characters common to the end of the first
    //  *     string and the start of the second string.
    //  */
    // protected int diff_commonOverlap(string text1, string text2) {
    //   // Cache the text lengths to prevent multiple calls.
    //   int text1_length = text1.Length;
    //   int text2_length = text2.Length;
    //   // Eliminate the null case.
    //   if (text1_length == 0 || text2_length == 0) {
    //     return 0;
    //   }
    //   // Truncate the longer string.
    //   if (text1_length > text2_length) {
    //     text1 = text1.Substring(text1_length - text2_length);
    //   } else if (text1_length < text2_length) {
    //     text2 = text2.Substring(0, text1_length);
    //   }
    //   int text_length = Math.Min(text1_length, text2_length);
    //   // Quick check for the worst case.
    //   if (text1 == text2) {
    //     return text_length;
    //   }

    //   // Start by looking for a single character match
    //   // and increase length until no match is found.
    //   // Performance analysis: https://neil.fraser.name/news/2010/11/04/
    //   int best = 0;
    //   int length = 1;
    //   while (true) {
    //     string pattern = text1.Substring(text_length - length);
    //     int found = text2.IndexOf(pattern, StringComparison.Ordinal);
    //     if (found == -1) {
    //       return best;
    //     }
    //     length += found;
    //     if (found == 0 || text1.Substring(text_length - length) ==
    //         text2.Substring(0, length)) {
    //       best = length;
    //       length++;
    //     }
    //   }
    // }



    

    // /**
    //  * Reduce the number of edits by eliminating semantically trivial
    //  * equalities.
    //  * @param diffs List of Diff objects.
    //  */
    // public void diff_cleanupSemantic(List<Diff> diffs) {
    //   bool changes = false;
    //   // Stack of indices where equalities are found.
    //   Stack<int> equalities = new Stack<int>();
    //   // Always equal to equalities[equalitiesLength-1][1]
    //   string lastEquality = null;
    //   int pointer = 0;  // Index of current position.
    //   // Number of characters that changed prior to the equality.
    //   int length_insertions1 = 0;
    //   int length_deletions1 = 0;
    //   // Number of characters that changed after the equality.
    //   int length_insertions2 = 0;
    //   int length_deletions2 = 0;
    //   while (pointer < diffs.Count) {
    //     if (diffs[pointer].operation == Operation.EQUAL) {  // Equality found.
    //       equalities.Push(pointer);
    //       length_insertions1 = length_insertions2;
    //       length_deletions1 = length_deletions2;
    //       length_insertions2 = 0;
    //       length_deletions2 = 0;
    //       lastEquality = diffs[pointer].text;
    //     } else {  // an insertion or deletion
    //       if (diffs[pointer].operation == Operation.INSERT) {
    //         length_insertions2 += diffs[pointer].text.Length;
    //       } else {
    //         length_deletions2 += diffs[pointer].text.Length;
    //       }
    //       // Eliminate an equality that is smaller or equal to the edits on both
    //       // sides of it.
    //       if (lastEquality != null && (lastEquality.Length
    //           <= Math.Max(length_insertions1, length_deletions1))
    //           && (lastEquality.Length
    //               <= Math.Max(length_insertions2, length_deletions2))) {
    //         // Duplicate record.
    //         diffs.Insert(equalities.Peek(),
    //                      new Diff(Operation.DELETE, lastEquality));
    //         // Change second copy to insert.
    //         diffs[equalities.Peek() + 1].operation = Operation.INSERT;
    //         // Throw away the equality we just deleted.
    //         equalities.Pop();
    //         if (equalities.Count > 0) {
    //           equalities.Pop();
    //         }
    //         pointer = equalities.Count > 0 ? equalities.Peek() : -1;
    //         length_insertions1 = 0;  // Reset the counters.
    //         length_deletions1 = 0;
    //         length_insertions2 = 0;
    //         length_deletions2 = 0;
    //         lastEquality = null;
    //         changes = true;
    //       }
    //     }
    //     pointer++;
    //   }

    //   // Normalize the diff.
    //   if (changes) {
    //     diff_cleanupMerge(diffs);
    //   }
    //   diff_cleanupSemanticLossless(diffs);

    //   // Find any overlaps between deletions and insertions.
    //   // e.g: <del>abcxxx</del><ins>xxxdef</ins>
    //   //   -> <del>abc</del>xxx<ins>def</ins>
    //   // e.g: <del>xxxabc</del><ins>defxxx</ins>
    //   //   -> <ins>def</ins>xxx<del>abc</del>
    //   // Only extract an overlap if it is as big as the edit ahead or behind it.
    //   pointer = 1;
    //   while (pointer < diffs.Count) {
    //     if (diffs[pointer - 1].operation == Operation.DELETE &&
    //         diffs[pointer].operation == Operation.INSERT) {
    //       string deletion = diffs[pointer - 1].text;
    //       string insertion = diffs[pointer].text;
    //       int overlap_length1 = diff_commonOverlap(deletion, insertion);
    //       int overlap_length2 = diff_commonOverlap(insertion, deletion);
    //       if (overlap_length1 >= overlap_length2) {
    //         if (overlap_length1 >= deletion.Length / 2.0 ||
    //             overlap_length1 >= insertion.Length / 2.0) {
    //           // Overlap found.
    //           // Insert an equality and trim the surrounding edits.
    //           diffs.Insert(pointer, new Diff(Operation.EQUAL,
    //               insertion.Substring(0, overlap_length1)));
    //           diffs[pointer - 1].text =
    //               deletion.Substring(0, deletion.Length - overlap_length1);
    //           diffs[pointer + 1].text = insertion.Substring(overlap_length1);
    //           pointer++;
    //         }
    //       } else {
    //         if (overlap_length2 >= deletion.Length / 2.0 ||
    //             overlap_length2 >= insertion.Length / 2.0) {
    //           // Reverse overlap found.
    //           // Insert an equality and swap and trim the surrounding edits.
    //           diffs.Insert(pointer, new Diff(Operation.EQUAL,
    //               deletion.Substring(0, overlap_length2)));
    //           diffs[pointer - 1].operation = Operation.INSERT;
    //           diffs[pointer - 1].text =
    //               insertion.Substring(0, insertion.Length - overlap_length2);
    //           diffs[pointer + 1].operation = Operation.DELETE;
    //           diffs[pointer + 1].text = deletion.Substring(overlap_length2);
    //           pointer++;
    //         }
    //       }
    //       pointer++;
    //     }
    //     pointer++;
    //   }
    // }

    // /**
    //  * Look for single edits surrounded on both sides by equalities
    //  * which can be shifted sideways to align the edit to a word boundary.
    //  * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
    //  * @param diffs List of Diff objects.
    //  */
    // public void diff_cleanupSemanticLossless(List<Diff> diffs) {
    //   int pointer = 1;
    //   // Intentionally ignore the first and last element (don't need checking).
    //   while (pointer < diffs.Count - 1) {
    //     if (diffs[pointer - 1].operation == Operation.EQUAL &&
    //       diffs[pointer + 1].operation == Operation.EQUAL) {
    //       // This is a single edit surrounded by equalities.
    //       string equality1 = diffs[pointer - 1].text;
    //       string edit = diffs[pointer].text;
    //       string equality2 = diffs[pointer + 1].text;

    //       // First, shift the edit as far left as possible.
    //       int commonOffset = this.diff_commonSuffix(equality1, edit);
    //       if (commonOffset > 0) {
    //         string commonString = edit.Substring(edit.Length - commonOffset);
    //         equality1 = equality1.Substring(0, equality1.Length - commonOffset);
    //         edit = commonString + edit.Substring(0, edit.Length - commonOffset);
    //         equality2 = commonString + equality2;
    //       }

    //       // Second, step character by character right,
    //       // looking for the best fit.
    //       string bestEquality1 = equality1;
    //       string bestEdit = edit;
    //       string bestEquality2 = equality2;
    //       int bestScore = diff_cleanupSemanticScore(equality1, edit) +
    //           diff_cleanupSemanticScore(edit, equality2);
    //       while (edit.Length != 0 && equality2.Length != 0
    //           && edit[0] == equality2[0]) {
    //         equality1 += edit[0];
    //         edit = edit.Substring(1) + equality2[0];
    //         equality2 = equality2.Substring(1);
    //         int score = diff_cleanupSemanticScore(equality1, edit) +
    //             diff_cleanupSemanticScore(edit, equality2);
    //         // The >= encourages trailing rather than leading whitespace on
    //         // edits.
    //         if (score >= bestScore) {
    //           bestScore = score;
    //           bestEquality1 = equality1;
    //           bestEdit = edit;
    //           bestEquality2 = equality2;
    //         }
    //       }

    //       if (diffs[pointer - 1].text != bestEquality1) {
    //         // We have an improvement, save it back to the diff.
    //         if (bestEquality1.Length != 0) {
    //           diffs[pointer - 1].text = bestEquality1;
    //         } else {
    //           diffs.RemoveAt(pointer - 1);
    //           pointer--;
    //         }
    //         diffs[pointer].text = bestEdit;
    //         if (bestEquality2.Length != 0) {
    //           diffs[pointer + 1].text = bestEquality2;
    //         } else {
    //           diffs.RemoveAt(pointer + 1);
    //           pointer--;
    //         }
    //       }
    //     }
    //     pointer++;
    //   }
    // }

    // /**
    //  * Given two strings, compute a score representing whether the internal
    //  * boundary falls on logical boundaries.
    //  * Scores range from 6 (best) to 0 (worst).
    //  * @param one First string.
    //  * @param two Second string.
    //  * @return The score.
    //  */
    // private int diff_cleanupSemanticScore(string one, string two) {
    //   if (one.Length == 0 || two.Length == 0) {
    //     // Edges are the best.
    //     return 6;
    //   }

    //   // Each port of this function behaves slightly differently due to
    //   // subtle differences in each language's definition of things like
    //   // 'whitespace'.  Since this function's purpose is largely cosmetic,
    //   // the choice has been made to use each language's native features
    //   // rather than force total conformity.
    //   char char1 = one[one.Length - 1];
    //   char char2 = two[0];
    //   bool nonAlphaNumeric1 = !Char.IsLetterOrDigit(char1);
    //   bool nonAlphaNumeric2 = !Char.IsLetterOrDigit(char2);
    //   bool whitespace1 = nonAlphaNumeric1 && Char.IsWhiteSpace(char1);
    //   bool whitespace2 = nonAlphaNumeric2 && Char.IsWhiteSpace(char2);
    //   bool lineBreak1 = whitespace1 && Char.IsControl(char1);
    //   bool lineBreak2 = whitespace2 && Char.IsControl(char2);
    //   bool blankLine1 = lineBreak1 && BLANKLINEEND.IsMatch(one);
    //   bool blankLine2 = lineBreak2 && BLANKLINESTART.IsMatch(two);

    //   if (blankLine1 || blankLine2) {
    //     // Five points for blank lines.
    //     return 5;
    //   } else if (lineBreak1 || lineBreak2) {
    //     // Four points for line breaks.
    //     return 4;
    //   } else if (nonAlphaNumeric1 && !whitespace1 && whitespace2) {
    //     // Three points for end of sentences.
    //     return 3;
    //   } else if (whitespace1 || whitespace2) {
    //     // Two points for whitespace.
    //     return 2;
    //   } else if (nonAlphaNumeric1 || nonAlphaNumeric2) {
    //     // One point for non-alphanumeric.
    //     return 1;
    //   }
    //   return 0;
    // }

    // // Define some regex patterns for matching boundaries.
    // private Regex BLANKLINEEND = new Regex("\\n\\r?\\n\\Z");
    // private Regex BLANKLINESTART = new Regex("\\A\\r?\\n\\r?\\n");

    // /**
    //  * Reduce the number of edits by eliminating operationally trivial
    //  * equalities.
    //  * @param diffs List of Diff objects.
    //  */
    // public void diff_cleanupEfficiency(List<Diff> diffs) {
    //   bool changes = false;
    //   // Stack of indices where equalities are found.
    //   Stack<int> equalities = new Stack<int>();
    //   // Always equal to equalities[equalitiesLength-1][1]
    //   string lastEquality = string.Empty;
    //   int pointer = 0;  // Index of current position.
    //   // Is there an insertion operation before the last equality.
    //   bool pre_ins = false;
    //   // Is there a deletion operation before the last equality.
    //   bool pre_del = false;
    //   // Is there an insertion operation after the last equality.
    //   bool post_ins = false;
    //   // Is there a deletion operation after the last equality.
    //   bool post_del = false;
    //   while (pointer < diffs.Count) {
    //     if (diffs[pointer].operation == Operation.EQUAL) {  // Equality found.
    //       if (diffs[pointer].text.Length < this.Diff_EditCost
    //           && (post_ins || post_del)) {
    //         // Candidate found.
    //         equalities.Push(pointer);
    //         pre_ins = post_ins;
    //         pre_del = post_del;
    //         lastEquality = diffs[pointer].text;
    //       } else {
    //         // Not a candidate, and can never become one.
    //         equalities.Clear();
    //         lastEquality = string.Empty;
    //       }
    //       post_ins = post_del = false;
    //     } else {  // An insertion or deletion.
    //       if (diffs[pointer].operation == Operation.DELETE) {
    //         post_del = true;
    //       } else {
    //         post_ins = true;
    //       }
    //       /*
    //        * Five types to be split:
    //        * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
    //        * <ins>A</ins>X<ins>C</ins><del>D</del>
    //        * <ins>A</ins><del>B</del>X<ins>C</ins>
    //        * <ins>A</del>X<ins>C</ins><del>D</del>
    //        * <ins>A</ins><del>B</del>X<del>C</del>
    //        */
    //       if ((lastEquality.Length != 0)
    //           && ((pre_ins && pre_del && post_ins && post_del)
    //           || ((lastEquality.Length < this.Diff_EditCost / 2)
    //           && ((pre_ins ? 1 : 0) + (pre_del ? 1 : 0) + (post_ins ? 1 : 0)
    //           + (post_del ? 1 : 0)) == 3))) {
    //         // Duplicate record.
    //         diffs.Insert(equalities.Peek(),
    //                      new Diff(Operation.DELETE, lastEquality));
    //         // Change second copy to insert.
    //         diffs[equalities.Peek() + 1].operation = Operation.INSERT;
    //         equalities.Pop();  // Throw away the equality we just deleted.
    //         lastEquality = string.Empty;
    //         if (pre_ins && pre_del) {
    //           // No changes made which could affect previous entry, keep going.
    //           post_ins = post_del = true;
    //           equalities.Clear();
    //         } else {
    //           if (equalities.Count > 0) {
    //             equalities.Pop();
    //           }

    //           pointer = equalities.Count > 0 ? equalities.Peek() : -1;
    //           post_ins = post_del = false;
    //         }
    //         changes = true;
    //       }
    //     }
    //     pointer++;
    //   }

    //   if (changes) {
    //     diff_cleanupMerge(diffs);
    //   }
    // }

/**
 * Given the location of the 'middle snake', split the diff in two parts
 * and recurse.
 * @param text1 Old string to be diffed.
 * @param text2 New string to be diffed.
 * @param x Index of split point in text1.
 * @param y Index of split point in text2.
 * @param deadline Time at which to bail if not yet complete.
 * @return LinkedList of Diff objects.
 */
int diff_bisectSplit(vector<XChar>& char_list_x, int start_x, int end_x, 
                     vector<XChar>& char_list_y, int start_y, int end_y, 
                     int x, int y, vector<XChar>& return_list_x, vector<XChar>& return_list_y, int & similarity) 
{
    diff_main(char_list_x, start_x, x-1, char_list_y, start_y, y-1, return_list_x, return_list_y, similarity);
    diff_main(char_list_x, x, end_x, char_list_y, y, end_y, return_list_x, return_list_y, similarity);
    return 0;
}

/**
 * Find the 'middle snake' of a diff, split the problem in two
 * and return the recursively constructed diff.
 * See Myers 1986 paper: An O(ND) Difference Algorithm and Its Variations.
 * @param text1 Old string to be diffed.
 * @param text2 New string to be diffed.
 * @param deadline Time at which to bail if not yet complete.
 * @return List of Diff objects.
 */
int diff_bisect(vector<XChar>& char_list_x, int start_x, int end_x,
                vector<XChar>& char_list_y, int start_y, int end_y,
                vector<XChar>& return_list_x,
                vector<XChar>& return_list_y, int& similarity) 
{
    // Cache the text lengths to prevent multiple calls.
    int text1_length = end_x - start_x + 1;
    int text2_length = end_y - start_y + 1;
    int max_d = (text1_length + text2_length + 1) / 2;
    int v_offset = max_d;
    int v_length = 2 * max_d;
    vector<int> v1(v_length, -1);
    vector<int> v2(v_length, -1);
    v1[v_offset + 1] = 0;
    v2[v_offset + 1] = 0;

    int delta = text1_length - text2_length;
    // If the total number of characters is odd, then the front path will
    // collide with the reverse path.
    bool front = (delta % 2 != 0);
    // Offsets for start and end of k loop.
    // Prevents mapping of space beyond the grid.
    int k1start = 0;
    int k1end = 0;
    int k2start = 0;
    int k2end = 0;
    for (int d = 0; d < max_d; d++)
    {
        // Walk the front path one step.
        for (int k1 = -d + k1start; k1 <= d - k1end; k1 += 2)
        {
            int k1_offset = v_offset + k1;
            int x1;
            if (k1 == -d || k1 != d && v1[k1_offset - 1] < v1[k1_offset + 1])
            {
                x1 = v1[k1_offset + 1];
            }
            else
            {
                x1 = v1[k1_offset - 1] + 1;
            }
            int y1 = x1 - k1;
            while (x1 < text1_length && y1 < text2_length && char_list_x[start_x + x1].text == char_list_y[start_y + y1].text)
            {
                x1++;
                y1++;
            }
            v1[k1_offset] = x1;
            if (x1 > text1_length)
            {
                // Ran off the right of the graph.
                k1end += 2;
            }
            else if (y1 > text2_length)
            {
                // Ran off the bottom of the graph.
                k1start += 2;
            }
            else if (front)
            {
                int k2_offset = v_offset + delta - k1;
                if (k2_offset >= 0 && k2_offset < v_length && v2[k2_offset] != -1)
                {
                    // Mirror x2 onto top-left coordinate system.
                    int x2 = text1_length - v2[k2_offset];
                    if (x1 >= x2)
                    {
                        // Overlap detected.
                        return diff_bisectSplit(char_list_x, start_x, end_x, 
                                                char_list_y, start_y, end_y, 
                                                start_x+x1, start_y + y1,
                                                return_list_x, return_list_y, similarity);
                    }
                }
            }
        }

        // Walk the reverse path one step.
        for (int k2 = -d + k2start; k2 <= d - k2end; k2 += 2)
        {
            int k2_offset = v_offset + k2;
            int x2;
            if (k2 == -d || k2 != d && v2[k2_offset - 1] < v2[k2_offset + 1])
            {
                x2 = v2[k2_offset + 1];
            }
            else
            {
                x2 = v2[k2_offset - 1] + 1;
            }
            int y2 = x2 - k2;
            while (x2 < text1_length && y2 < text2_length && char_list_x[end_x - x2].text == char_list_y[end_y - y2].text)
            {
                x2++;
                y2++;
            }
            v2[k2_offset] = x2;
            if (x2 > text1_length)
            {
                // Ran off the left of the graph.
                k2end += 2;
            }
            else if (y2 > text2_length)
            {
                // Ran off the top of the graph.
                k2start += 2;
            }
            else if (!front)
            {
                int k1_offset = v_offset + delta - k2;
                if (k1_offset >= 0 && k1_offset < v_length && v1[k1_offset] != -1)
                {
                    int x1 = v1[k1_offset];
                    int y1 = v_offset + x1 - k1_offset;
                    // Mirror x2 onto top-left coordinate system.
                    x2 = text1_length - v2[k2_offset];
                    if (x1 >= x2)
                    {
                        // Overlap detected.
                        return diff_bisectSplit(char_list_x, start_x, end_x, 
                                                char_list_y, start_y, end_y, 
                                                start_x+x1, start_y+y1,
                                                return_list_x, return_list_y, similarity);
                    }
                }
            }
        }
    }
    // Diff took too long and hit the deadline or
    // number of diffs equals number of characters, no commonality at all.


    int min_sz = min(text1_length, text2_length);
    int i = 0;
    for(i = 0; i < min_sz; i++)
    {
        char_list_x[start_x+i].char_status = CHANGE;
        char_list_y[start_y+i].char_status = CHANGE;
        return_list_x.push_back(char_list_x[start_x+i]);
        return_list_y.push_back(char_list_y[start_y+i]);
    }

    while(start_x + i <= end_x)
    {
        char_list_x[start_x+i].char_status = DELETE;
        return_list_x.push_back(char_list_x[start_x + i++]);

        return_list_y.push_back(del_char);
    }

    while(start_y + i <= end_y)
    {
        char_list_y[start_y+i].char_status = INSERT;
        return_list_y.push_back(char_list_y[start_y + i++]);

        return_list_x.push_back(add_char);
    }
    return 0;
}


bool equal(vector<XChar>& char_list_x, int start_x, int end_x, 
           vector<XChar>& char_list_y, int start_y, int end_y)
{
    if(end_x - start_x != end_y - start_y)
        return false;

    int sz = end_x - start_x + 1;

    for(int i = 0; i < sz; i++)
    {
        if(char_list_x[start_x + i].text != char_list_y[start_y + i].text)
        {
            return false;
        }
    }

    return true;
}

int find(std::vector<XChar>& char_list_x, int start_x, int end_x, 
         std::vector<XChar>& char_list_y, int start_y, int end_y)
{
    int len_x = end_x - start_x + 1;
    int len_y = end_y - start_y + 1;

    if(len_x > len_y)
    {
        for(int i = 0; i < len_x-len_y+1; i++)
        {
            if(equal(char_list_x, start_x+i, start_x+i+len_y-1, char_list_y, start_y, end_y))
            {
                return start_x+i;
            }
        }
    }
    else
    {
        for(int i = 0; i < len_y - len_x+1; i++)
        {
            if(equal(char_list_x, start_x, end_x, char_list_y, start_y+i, start_y+i+len_x-1))
            {
                return start_y+i;
            }
        }
    }
    return -1;
}




/**
 * Find the differences between two texts.  Assumes that the texts do not
 * have any common prefix or suffix.
 * @param text1 Old string to be diffed.
 * @param text2 New string to be diffed.
 * @param checklines Speedup flag.  If false, then don't run a
 *     line-level diff first to identify the changed areas.
 *     If true, then run a faster slightly less optimal diff.
 * @param deadline Time when the diff should be complete by.
 * @return List of Diff objects.
 */


int diff_compute(std::vector<XChar>& char_list_x, int start_x, int end_x,
                 std::vector<XChar>& char_list_y, int start_y, int end_y,
                 std::vector<XChar>& return_list_x, 
                 std::vector<XChar>& return_list_y, int& similarity) 
{

        if(end_x - start_x + 1 <= 0 && end_y - start_y + 1 <= 0)
    {
        return 0;
    }
    else if (end_x - start_x + 1 <= 0) 
    {
        for(int i = start_y; i <= end_y; i++)
        {
            XChar add_char;
            add_char.char_status = INSERT;
            add_char.box.assign(-1, -1, -1, -1);
            return_list_x.push_back(std::move(add_char));

            char_list_y[i].char_status = INSERT;
            return_list_y.push_back(char_list_y[i]);
        }
        return 0;
    }
    else if(end_y - start_y + 1 <= 0) 
    {
        for(int i = start_x; i <= end_x; i++)
        {
            XChar del_char;
            del_char.char_status = DELETE;
            del_char.box.assign(-1, -1, -1, -1);
            return_list_y.push_back(std::move(del_char));

            char_list_x[i].char_status = DELETE;
            return_list_x.push_back(char_list_x[i]);
        }
        return 0;
    }












    int len_x = end_x - start_x + 1;
    int len_y = end_y - start_y + 1;

    int pos = find(char_list_x, start_x, end_x, char_list_y, start_y, end_y);
    if(pos != -1)
    {
        if(len_x > len_y)
        {
            for(int i = start_x; i < pos; i++)
            {
                char_list_x[i].char_status = DELETE;
                return_list_x.push_back(char_list_x[i]);

                XChar add_char;
                add_char.char_status = DELETE;
                add_char.box.assign(-1, -1, -1, -1);
                return_list_y.push_back(std::move(add_char));
            }

            for(int i = 0; i < end_y-start_y+1; i++)
            {
                return_list_x.push_back(char_list_x[pos+i]);
                return_list_y.push_back(char_list_y[start_y+i]);
                similarity++;
            }

            for(int i = pos + len_y; i <= end_x; i++)
            {
                char_list_x[i].char_status = DELETE;
                return_list_x.push_back(char_list_x[i]);

                XChar add_char;
                add_char.char_status = DELETE;
                add_char.box.assign(-1, -1, -1, -1);
                return_list_y.push_back(std::move(add_char));
            }
        }
        else
        {
            for(int i = start_y; i < pos; i++)
            {
                char_list_y[i].char_status = INSERT;
                return_list_y.push_back(char_list_y[i]);

                XChar add_char;
                add_char.char_status = INSERT;
                add_char.box.assign(-1, -1, -1, -1);
                return_list_x.push_back(std::move(add_char));
            }

            for(int i = 0; i < len_x; i++)
            {
                return_list_y.push_back(char_list_y[pos+i]);
                return_list_x.push_back(char_list_x[start_x+i]);
                similarity++;
            }

            for(int i = pos + len_x; i <= end_y; i++)
            {
                char_list_y[i].char_status = INSERT;
                return_list_y.push_back(char_list_y[i]);

                XChar add_char;
                add_char.char_status = INSERT;
                add_char.box.assign(-1, -1, -1, -1);
                return_list_x.push_back(std::move(add_char));
            }
        }
        return 0;
    }

    if(min(len_x, len_y) == 1)
    {
        char_list_x[start_x].char_status = CHANGE;
        char_list_y[start_y].char_status = CHANGE;

        return_list_x.push_back(char_list_x[start_x]);
        return_list_y.push_back(char_list_y[start_y]);

        int i = 1;
        while(start_x + i <= end_x)
        {
            char_list_x[start_x+i].char_status = DELETE;
            return_list_x.push_back(char_list_x[start_x + i++]);

            return_list_y.push_back(del_char);
        }

        while(start_y + i <= end_y)
        {
            return_list_x.push_back(add_char);

            char_list_y[start_y + i].char_status = INSERT;
            return_list_y.push_back(char_list_y[start_y + i++]);
        }
        return 0;
    }

    return diff_bisect(char_list_x, start_x, end_x, char_list_y, start_y, end_y,
                    return_list_x, return_list_y, similarity);
}








/**
 * Find the differences between two texts.  Simplifies the problem by
 * stripping any common prefix or suffix off the texts before diffing.
 * @param text1 Old string to be diffed.
 * @param text2 New string to be diffed.
 * @return List of Diff objects.
 */

int diff_main(std::vector<XChar>& char_list_x, int start_x, int end_x,
              std::vector<XChar>& char_list_y, int start_y, int end_y,
              std::vector<XChar>& return_list_x, 
              std::vector<XChar>& return_list_y, int& similarity)
{
    if(end_x - start_x + 1 <= 0 && end_y - start_y + 1 <= 0)
    {
        return 0;
    }
    else if (end_x - start_x + 1 <= 0) 
    {
        for(int i = start_y; i <= end_y; i++)
        {
            XChar add_char;
            add_char.char_status = INSERT;
            add_char.box.assign(-1, -1, -1, -1);
            return_list_x.push_back(std::move(add_char));

            char_list_y[i].char_status = INSERT;
            return_list_y.push_back(char_list_y[i]);
        }
        return 0;
    }
    else if(end_y - start_y + 1 <= 0) 
    {
        for(int i = start_x; i <= end_x; i++)
        {
            XChar del_char;
            del_char.char_status = DELETE;
            del_char.box.assign(-1, -1, -1, -1);
            return_list_y.push_back(std::move(del_char));

            char_list_x[i].char_status = DELETE;
            return_list_x.push_back(char_list_x[i]);
        }
        return 0;
    }



    // Check for null inputs not needed since null can't be passed in C#.

    // Check for equality (speedup).
    if (equal(char_list_x, start_x, end_x, char_list_y, start_y, end_y)) 
    {
        int sz = end_x - start_x + 1;
        for(int i = 0; i < sz; i++)
        {
            return_list_x.emplace_back(char_list_x[start_x + i]);
            return_list_y.emplace_back(char_list_y[start_y + i]);
            similarity++;
        }

        return 0;
    }

    // Trim off common prefix (speedup).
    int commonlength = diff_commonPrefix(char_list_x, start_x, end_x, char_list_y, start_y, end_y);
    if(commonlength > 0)
    {
        for(int i = 0; i < commonlength; i++)
        {
            return_list_x.emplace_back(char_list_x[start_x + i]);
            return_list_y.emplace_back(char_list_y[start_y + i]);
            similarity++;
        }

        start_x += commonlength;
        start_y += commonlength;
    }


    // Trim off common suffix (speedup).
    commonlength = diff_commonSuffix(char_list_x, start_x, end_x, char_list_y, start_y, end_y);


    int tmp_end_x = end_x;
    int tmp_end_y = end_y;


    if(commonlength > 0)
    {
        end_x -= commonlength;
        end_y -= commonlength;
    }


    diff_compute(char_list_x, start_x, end_x, 
                 char_list_y, start_y, end_y, 
                 return_list_x, return_list_y, similarity);

    if(commonlength > 0)
    {
        for(int i = 0; i < commonlength; i++)
        {
            return_list_x.emplace_back(char_list_x[tmp_end_x - commonlength + 1 + i]);
            return_list_y.emplace_back(char_list_y[tmp_end_y - commonlength + 1 + i]);
            similarity++;
        }
    }


    return 0;
}

int diff_main_call(std::vector<XChar>& char_x,
              std::vector<XChar>& char_y,
              std::vector<XChar>& return_list1, 
              std::vector<XChar>& return_list2, int& similarity)
{
    add_char.char_status = INSERT;
    add_char.box.assign(-1, -1, -1, -1);

    del_char.char_status = DELETE;
    del_char.box.assign(-1, -1, -1, -1);

    return diff_main(char_x, 0, char_x.size()-1, 
                     char_y, 0, char_y.size()-1, 
                     return_list1, return_list2, similarity);
}


} // namespace diff