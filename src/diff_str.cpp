#include <vector>
#include <string>
#include <list>
#include <time.h>
#include <limits>
#include <map>
#include <stack>
#include <sstream>
#include <regex>
#include "diff.h"

using std::list;
using std::map;
using std::stack;
using std::vector;
using std::wstring;

namespace xwb
{
    //   internal static class CompatibilityExtensions {
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

    vector<Diff> diff_bisectSplit(const wstring &text1, const wstring &text2, int x, int y, clock_t deadline);
    void diff_cleanupSemantic(vector<Diff>& diffs);
    vector<Diff> diff_bisect(wstring &text1, wstring &text2, clock_t deadline);
    void diff_cleanupMerge(vector<Diff>& diffs);
    void diff_cleanupSemanticLossless(vector<Diff>& diffs);
    int diff_cleanupSemanticScore(wstring one, wstring two);

    /**
   * Class containing the diff, match and patch methods.
   * Also Contains the behaviour settings.
   */

    // Defaults.
    // Set these on your diff_match_patch instance to override the defaults.

    // Number of seconds to map a diff before giving up (0 for infinity).
    const float Diff_Timeout = -1.0f;
    // Cost of an empty edit operation in terms of edit characters.
    const short Diff_EditCost = 4;
    // At what point is no match declared (0.0 = perfection, 1.0 = very loose).
    const float Match_Threshold = 0.5f;
    // How far to search for a match (0 = exact location, 1000+ = broad match).
    // A match this many characters away from the expected location will add
    // 1.0 to the score (0.0 is a perfect match).
    const int Match_Distance = 1000;
    // When deleting a large block of text (over ~64 characters), how close
    // do the contents have to be to match the expected contents. (0.0 =
    // perfection, 1.0 = very loose).  Note that Match_Threshold controls
    // how closely the end points of a delete need to match.
    const float Patch_DeleteThreshold = 0.5f;
    // Chunk size for context length.
    const short Patch_Margin = 4;

    // The number of bits in an int.
    const short Match_MaxBits = 32;

    // 是否合并差异项
    static bool st_merge_diff = true;
    //  DIFF FUNCTIONS
    /**
     * Does a Substring of shorttext exist within longtext such that the
     * Substring is at least half the length of longtext?
     * @param longtext Longer string.
     * @param shorttext Shorter string.
     * @param i Start index of quarter length Substring within longtext.
     * @return Five element string array, containing the prefix of longtext, the
     *     suffix of longtext, the prefix of shorttext, the suffix of shorttext
     *     and the common middle.  Or null if there was no match.
     */

    /**
     * Determine the common prefix of two strings.
     * @param text1 First string.
     * @param text2 Second string.
     * @return The number of characters common to the start of each string.
     */
    int diff_commonPrefix(const wstring &text1, const wstring &text2)
    {
        // Performance analysis: https://neil.fraser.name/news/2007/10/09/
        int n = std::min(text1.size(), text2.size());
        for (int i = 0; i < n; i++)
        {
            if (text1[i] != text2[i])
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
    int diff_commonSuffix(const wstring &text1, const wstring &text2)
    {
        // Performance analysis: https://neil.fraser.name/news/2007/10/09/
        int text1_length = text1.size();
        int text2_length = text2.size();
        int n = std::min(text1.size(), text2.size());
        for (int i = 1; i <= n; i++)
        {
            if (text1[text1_length - i] != text2[text2_length - i])
            {
                return i - 1;
            }
        }
        return n;
    }

    /**
     * Determine if the suffix of one string is the prefix of another.
     * @param text1 First string.
     * @param text2 Second string.
     * @return The number of characters common to the end of the first
     *     string and the start of the second string.
     */
    int diff_commonOverlap(wstring text1, wstring text2)
    {
        // Cache the text lengths to prevent multiple calls.
        int text1_length = text1.size();
        int text2_length = text2.size();
        // Eliminate the null case.
        if (text1_length == 0 || text2_length == 0)
        {
            return 0;
        }
        // Truncate the longer string.
        if (text1_length > text2_length)
        {
            text1 = text1.substr(text1_length - text2_length);
        }
        else if (text1_length < text2_length)
        {
            text2 = text2.substr(0, text1_length);
        }
        int text_length = std::min(text1_length, text2_length);
        // Quick check for the worst case.
        if (text1 == text2)
        {
            return text_length;
        }

        // Start by looking for a single character match
        // and increase length until no match is found.
        // Performance analysis: https://neil.fraser.name/news/2010/11/04/
        int best = 0;
        int length = 1;
        while (true)
        {
            wstring pattern = text1.substr(text_length - length);
            int found = text2.find(pattern);
            if (found == -1)
            {
                return best;
            }
            length += found;
            if (found == 0 || text1.substr(text_length - length) == text2.substr(0, length))
            {
                best = length;
                length++;
            }
        }
    }

    vector<wstring> diff_halfMatchI(wstring longtext, wstring shorttext, int i)
    {
        // Start with a 1/4 length Substring at position i as a seed.
        wstring seed = longtext.substr(i, longtext.size() / 4);
        int j = -1;
        wstring best_common;
        wstring best_longtext_a, best_longtext_b;
        wstring best_shorttext_a, best_shorttext_b;

        while (j < shorttext.size() && (j = shorttext.find(seed, j + 1)) != -1)
        {
            int prefixLength = diff_commonPrefix(longtext.substr(i), shorttext.substr(j));
            int suffixLength = diff_commonSuffix(longtext.substr(0, i), shorttext.substr(0, j));
            if (best_common.size() < suffixLength + prefixLength)
            {
                best_common = shorttext.substr(j - suffixLength, suffixLength) + shorttext.substr(j, prefixLength);
                best_longtext_a = longtext.substr(0, i - suffixLength);
                best_longtext_b = longtext.substr(i + prefixLength);
                best_shorttext_a = shorttext.substr(0, j - suffixLength);
                best_shorttext_b = shorttext.substr(j + prefixLength);
            }
        }
        if (best_common.size() * 2 >= longtext.size())
        {
            return vector<wstring>{best_longtext_a, best_longtext_b,
                                   best_shorttext_a, best_shorttext_b, best_common};
        }
        else
        {
            return vector<wstring>();
        }
    }

    /**
     * Do the two texts share a Substring which is at least half the length of
     * the longer text?
     * This speedup can produce non-minimal diffs.
     * @param text1 First string.
     * @param text2 Second string.
     * @return Five element String array, containing the prefix of text1, the
     *     suffix of text1, the prefix of text2, the suffix of text2 and the
     *     common middle.  Or null if there was no match.
     */

    vector<wstring> diff_halfMatch(const std::wstring &text1, const std::wstring &text2)
    {
        vector<wstring> hm;
        if (Diff_Timeout <= 0)
        {
            // Don't risk returning a non-optimal diff if we have unlimited time.
            return hm;
        }
        wstring longtext = text1.size() > text2.size() ? text1 : text2;
        wstring shorttext = text1.size() > text2.size() ? text2 : text1;
        if (longtext.size() < 4 || shorttext.size() * 2 < longtext.size())
        {
            return hm; // Pointless.
        }

        // First check if the second quarter is the seed for a half-match.
        vector<wstring> hm1 = diff_halfMatchI(longtext, shorttext, (longtext.size() + 3) / 4);
        // Check again based on the third quarter.
        vector<wstring> hm2 = diff_halfMatchI(longtext, shorttext, (longtext.size() + 1) / 2);

        if (hm1.empty() && hm2.empty())
        {
            return hm;
        }
        else if (hm2.empty())
        {
            hm = hm1;
        }
        else if (hm1.empty())
        {
            hm = hm2;
        }
        else
        {
            // Both matched.  Select the longest.
            hm = hm1[4].size() > hm2[4].size() ? hm1 : hm2;
        }

        // A half-match was found, sort out the return data.
        if (text1.size() > text2.size())
        {
            return hm;
        }
        else
        {
            return vector<wstring>{hm[2], hm[3], hm[0], hm[1], hm[4]};
        }
    }

    struct LinesToCharsResult
    {
        std::wstring text1;
        std::wstring text2;
        list<wstring> linearray;
        /* data */
    };

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
    vector<Diff> diff_compute(wstring &text1, wstring &text2, bool checklines, clock_t deadline)
    {
        vector<Diff> diffs;

        if (text1.size() == 0)
        {
            // Just add some text (speedup).
            diffs.emplace_back(INSERT, text2);
            return diffs;
        }

        if (text2.size() == 0)
        {
            // Just delete some text (speedup).
            diffs.emplace_back(DELETE, text1);
            return diffs;
        }

        std::wstring longtext = text1.size() > text2.size() ? text1 : text2;
        std::wstring shorttext = text1.size() > text2.size() ? text2 : text1;
        int i = longtext.find(shorttext);
        if (i != -1)
        {
            // Shorter text is inside the longer text (speedup).
            CharStatus op = (text1.size() > text2.size()) ? DELETE : INSERT;
            diffs.emplace_back(op, longtext.substr(0, i));
            diffs.emplace_back(NORMAL, shorttext);
            diffs.emplace_back(op, longtext.substr(i + shorttext.size()));
            return diffs;
        }

        if (shorttext.size() == 1)
        {
            // Single character string.
            // After the previous speedup, the character can't be an equality.
            diffs.emplace_back(DELETE, text1);
            diffs.emplace_back(INSERT, text2);
            return diffs;
        }

        // Check to see if the problem can be split in two.
        vector<wstring> hm = diff_halfMatch(text1, text2);
        if (hm.size())
        {
            // A half-match was found, sort out the return data.
            wstring text1_a = hm[0];
            wstring text1_b = hm[1];
            wstring text2_a = hm[2];
            wstring text2_b = hm[3];
            wstring mid_common = hm[4];
            // Send both pairs off for separate processing.
            vector<Diff> diffs_a = diff_main(text1_a, text2_a, checklines, deadline);
            vector<Diff> diffs_b = diff_main(text1_b, text2_b, checklines, deadline);
            // Merge the results.
            diffs = diffs_a;
            diffs.emplace_back(NORMAL, mid_common);
            diffs.insert(diffs.begin(), diffs_b.begin(), diffs_b.end());
            return diffs;
        }

        return diff_bisect(text1, text2, deadline);
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
    vector<Diff> diff_bisect(wstring &text1, wstring &text2, clock_t deadline)
    {
        // Cache the text lengths to prevent multiple calls.
        int text1_length = text1.size();
        int text2_length = text2.size();
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
            // Bail out if deadline is reached.
            if (clock() > deadline)
            {
                break;
            }

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
                while (x1 < text1_length && y1 < text2_length && text1[x1] == text2[y1])
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
                            return diff_bisectSplit(text1, text2, x1, y1, deadline);
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
                while (x2 < text1_length && y2 < text2_length && text1[text1_length - x2 - 1] == text2[text2_length - y2 - 1])
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
                            return diff_bisectSplit(text1, text2, x1, y1, deadline);
                        }
                    }
                }
            }
        }
        // Diff took too long and hit the deadline or
        // number of diffs equals number of characters, no commonality at all.
        vector<Diff> diffs;
        diffs.emplace_back(DELETE, text1);
        diffs.emplace_back(INSERT, text2);
        return diffs;
    }

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
    vector<Diff> diff_bisectSplit(const wstring &text1, const wstring &text2, int x, int y, clock_t deadline)
    {
        wstring text1a = text1.substr(0, x);
        wstring text2a = text2.substr(0, y);
        wstring text1b = text1.substr(x);
        wstring text2b = text2.substr(y);

        // Compute both diffs serially.
        vector<Diff> diffs = diff_main(text1a, text2a, false, deadline);
        vector<Diff> diffsb = diff_main(text1b, text2b, false, deadline);

        diffs.insert(diffs.end(), diffsb.begin(), diffsb.end());
        return diffs;
    }

    /**
     * Split a text into a list of strings.  Reduce the texts to a string of
     * hashes where each Unicode character represents one line.
     * @param text String to encode.
     * @param lineArray List of unique strings.
     * @param lineHash Map of strings to indices.
     * @param maxLines Maximum length of lineArray.
     * @return Encoded string.
     */
    wstring diff_linesToCharsMunge(wstring text, list<wstring> lineArray, map<wstring, int> lineHash, int maxLines)
    {
        int lineStart = 0;
        int lineEnd = -1;
        wstring line;
        wstring chars;
        // Walk the text, pulling out a Substring for each line.
        // text.split('\n') would would temporarily double our memory footprint.
        // Modifying text would create many large strings to garbage collect.
        while (lineEnd < text.size() - 1)
        {
            lineEnd = text.find(L'\n', lineStart);
            if (lineEnd == -1)
            {
                lineEnd = text.size() - 1;
            }
            line = text.substr(lineStart, lineEnd - lineStart + 1);

            if (lineHash.count(line))
            {
                chars.append(1, ((wchar_t)(int)lineHash[line]));
            }
            else
            {
                if (lineArray.size() == maxLines)
                {
                    // Bail out at 65535 because char 65536 == char 0.
                    line = text.substr(lineStart);
                    lineEnd = text.size();
                }
                lineArray.emplace_back(line);
                lineHash.emplace(line, lineArray.size() - 1);
                chars.append(1, ((wchar_t)(lineArray.size() - 1)));
            }
            lineStart = lineEnd + 1;
        }
        return chars;
    }

    /**
     * Split two texts into a list of strings.  Reduce the texts to a string of
     * hashes where each Unicode character represents one line.
     * @param text1 First string.
     * @param text2 Second string.
     * @return Three element Object array, containing the encoded text1, the
     *     encoded text2 and the List of unique strings.  The zeroth element
     *     of the List of unique strings is intentionally blank.
     */
    LinesToCharsResult diff_linesToChars(wstring text1, wstring text2)
    {
        list<wstring> lineArray;
        std::map<wstring, int> lineHash;
        // e.g. linearray[4] == "Hello\n"
        // e.g. linehash.get("Hello\n") == 4

        // "\x00" is a valid character, but various debuggers don't like it.
        // So we'll insert a junk entry to avoid generating a null character.
        lineArray.emplace_back(wstring());

        // Allocate 2/3rds of the space for text1, the rest for text2.
        wstring chars1 = diff_linesToCharsMunge(text1, lineArray, lineHash, 40000);
        wstring chars2 = diff_linesToCharsMunge(text2, lineArray, lineHash, 65535);

        LinesToCharsResult result;
        result.text1 = chars1;
        result.text2 = chars2;
        result.linearray = lineArray;

        return result;
    }

    /**
     * Reduce the number of edits by eliminating semantically trivial
     * equalities.
     * @param diffs List of Diff objects.
     */
    void diff_cleanupSemantic(vector<Diff> &diffs)
    {
        bool changes = false;
        // Stack of indices where equalities are found.
        stack<int> equalities;
        // Always equal to equalities[equalitiesLength-1][1]
        wstring lastEquality;
        int pointer = 0; // Index of current position.
        // Number of characters that changed prior to the equality.
        int length_insertions1 = 0;
        int length_deletions1 = 0;
        // Number of characters that changed after the equality.
        int length_insertions2 = 0;
        int length_deletions2 = 0;
        while (pointer < diffs.size())
        {
            if (diffs[pointer].operation == NORMAL)
            { // Equality found.
                equalities.push(pointer);
                length_insertions1 = length_insertions2;
                length_deletions1 = length_deletions2;
                length_insertions2 = 0;
                length_deletions2 = 0;
                lastEquality = diffs[pointer].text;
            }
            else
            { // an insertion or deletion
                if (diffs[pointer].operation == INSERT)
                {
                    length_insertions2 += diffs[pointer].text.size();
                }
                else
                {
                    length_deletions2 += diffs[pointer].text.size();
                }
                // Eliminate an equality that is smaller or equal to the edits on both
                // sides of it.
                if (lastEquality.size()
                    && (lastEquality.size() <= std::max(length_insertions1, length_deletions1)) 
                    && (lastEquality.size() <= std::max(length_insertions2, length_deletions2))
                    && (st_merge_diff || std::regex_match(lastEquality, std::wregex(L"^[a-zA-Z0-9]*$"))))
                {
                    // Duplicate record.
                    diffs.insert(diffs.begin() + equalities.top(), Diff(DELETE, lastEquality));
                    // Change second copy to insert.
                    diffs[equalities.top() + 1].operation = INSERT;
                    // Throw away the equality we just deleted.
                    equalities.pop();
                    if (equalities.size() > 0)
                    {
                        equalities.pop();
                    }
                    pointer = equalities.size() > 0 ? equalities.top() : -1;
                    length_insertions1 = 0; // Reset the counters.
                    length_deletions1 = 0;
                    length_insertions2 = 0;
                    length_deletions2 = 0;
                    lastEquality.clear();
                    changes = true;
                }
            }
            pointer++;
        }

        // Normalize the diff.
        if (changes)
        {
            diff_cleanupMerge(diffs);
        }
        diff_cleanupSemanticLossless(diffs);

        // Find any overlaps between deletions and insertions.
        // e.g: <del>abcxxx</del><ins>xxxdef</ins>
        //   -> <del>abc</del>xxx<ins>def</ins>
        // e.g: <del>xxxabc</del><ins>defxxx</ins>
        //   -> <ins>def</ins>xxx<del>abc</del>
        // Only extract an overlap if it is as big as the edit ahead or behind it.
        pointer = 1;
        while (pointer < diffs.size())
        {
            if (diffs[pointer - 1].operation == DELETE && diffs[pointer].operation == INSERT)
            {
                wstring deletion = diffs[pointer - 1].text;
                wstring insertion = diffs[pointer].text;
                int overlap_length1 = diff_commonOverlap(deletion, insertion);
                int overlap_length2 = diff_commonOverlap(insertion, deletion);
                if (overlap_length1 >= overlap_length2)
                {
                    if (overlap_length1 >= deletion.size() / 2.0 ||
                        overlap_length1 >= insertion.size() / 2.0)
                    {
                        // Overlap found.
                        // Insert an equality and trim the surrounding edits.
                        diffs.insert(diffs.begin() + pointer, Diff(NORMAL, insertion.substr(0, overlap_length1)));
                        diffs[pointer - 1].text = deletion.substr(0, deletion.size() - overlap_length1);
                        diffs[pointer + 1].text = insertion.substr(overlap_length1);
                        pointer++;
                    }
                }
                else
                {
                    if (overlap_length2 >= deletion.size() / 2.0 ||
                        overlap_length2 >= insertion.size() / 2.0)
                    {
                        // Reverse overlap found.
                        // Insert an equality and swap and trim the surrounding edits.
                        diffs.insert(diffs.begin() + pointer, Diff(NORMAL, deletion.substr(0, overlap_length2)));
                        diffs[pointer - 1].operation = INSERT;
                        diffs[pointer - 1].text = insertion.substr(0, insertion.size() - overlap_length2);
                        diffs[pointer + 1].operation = DELETE;
                        diffs[pointer + 1].text = deletion.substr(overlap_length2);
                        pointer++;
                    }
                }
                pointer++;
            }
            pointer++;
        }
    }

    /**
     * Look for single edits surrounded on both sides by equalities
     * which can be shifted sideways to align the edit to a word boundary.
     * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
     * @param diffs List of Diff objects.
     */
    void diff_cleanupSemanticLossless(vector<Diff> &diffs)
    {
        int pointer = 1;
        // Intentionally ignore the first and last element (don't need checking).
        while (pointer < (int)diffs.size() - 1)
        {
            if (diffs[pointer - 1].operation == NORMAL &&
                diffs[pointer + 1].operation == NORMAL)
            {
                // This is a single edit surrounded by equalities.
                wstring equality1 = diffs[pointer - 1].text;
                wstring edit = diffs[pointer].text;
                wstring equality2 = diffs[pointer + 1].text;

                // First, shift the edit as far left as possible.
                int commonOffset = diff_commonSuffix(equality1, edit);
                if (commonOffset > 0)
                {
                    wstring commonString = edit.substr(edit.size() - commonOffset);
                    equality1 = equality1.substr(0, equality1.size() - commonOffset);
                    edit = commonString + edit.substr(0, edit.size() - commonOffset);
                    equality2 = commonString + equality2;
                }

                // Second, step character by character right,
                // looking for the best fit.
                wstring bestEquality1 = equality1;
                wstring bestEdit = edit;
                wstring bestEquality2 = equality2;
                int bestScore = diff_cleanupSemanticScore(equality1, edit) +
                                diff_cleanupSemanticScore(edit, equality2);
                while (edit.size() != 0 && equality2.size() != 0 && edit[0] == equality2[0])
                {
                    equality1 += edit[0];
                    edit = edit.substr(1) + equality2[0];
                    equality2 = equality2.substr(1);
                    int score = diff_cleanupSemanticScore(equality1, edit) +
                                diff_cleanupSemanticScore(edit, equality2);
                    // The >= encourages trailing rather than leading whitespace on
                    // edits.
                    if (score >= bestScore)
                    {
                        bestScore = score;
                        bestEquality1 = equality1;
                        bestEdit = edit;
                        bestEquality2 = equality2;
                    }
                }

                if (diffs[pointer - 1].text != bestEquality1)
                {
                    // We have an improvement, save it back to the diff.
                    if (bestEquality1.size() != 0)
                    {
                        diffs[pointer - 1].text = bestEquality1;
                    }
                    else
                    {
                        diffs.erase(diffs.begin() + pointer - 1);
                        pointer--;
                    }
                    diffs[pointer].text = bestEdit;
                    if (bestEquality1.size() != 0)
                    {
                        diffs[pointer + 1].text = bestEquality2;
                    }
                    else
                    {
                        diffs.erase(diffs.begin() + pointer + 1);
                        pointer--;
                    }
                }
            }
            pointer++;
        }
    }

    bool isLetterOrDigit(wchar_t c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }

    bool isWhiteSpace(wchar_t c)
    {
        return c == ' ' || c == '\n' || c == '\r';
    }

    bool isControl(wchar_t c)
    {
        return c < 32;
    }
    /**
     * Given two strings, compute a score representing whether the internal
     * boundary falls on logical boundaries.
     * Scores range from 6 (best) to 0 (worst).
     * @param one First string.
     * @param two Second string.
     * @return The score.
     */
    int diff_cleanupSemanticScore(wstring one, wstring two)
    {
        if (one.size() == 0 || two.size() == 0)
        {
            // Edges are the best.
            return 6;
        }

        // Each port of this function behaves slightly differently due to
        // subtle differences in each language's definition of things like
        // 'whitespace'.  Since this function's purpose is largely cosmetic,
        // the choice has been made to use each language's native features
        // rather than force total conformity.
        wchar_t char1 = one.back();
        wchar_t char2 = two[0];
        bool nonAlphaNumeric1 = isLetterOrDigit(char1);
        bool nonAlphaNumeric2 = isLetterOrDigit(char2);
        bool whitespace1 = nonAlphaNumeric1 && isWhiteSpace(char1);
        bool whitespace2 = nonAlphaNumeric2 && isWhiteSpace(char2);
        bool lineBreak1 = whitespace1 && isControl(char1);
        bool lineBreak2 = whitespace2 && isControl(char2);
        bool blankLine1 = lineBreak1 && std::regex_search(one, std::wregex(L"[\\r\\n]^"));
        bool blankLine2 = lineBreak2 && std::regex_search(two, std::wregex(L"^[\\r\\n]"));

        if (blankLine1 || blankLine2)
        {
            // Five points for blank lines.
            return 5;
        }
        else if (lineBreak1 || lineBreak2)
        {
            // Four points for line breaks.
            return 4;
        }
        else if (nonAlphaNumeric1 && !whitespace1 && whitespace2)
        {
            // Three points for end of sentences.
            return 3;
        }
        else if (whitespace1 || whitespace2)
        {
            // Two points for whitespace.
            return 2;
        }
        else if (nonAlphaNumeric1 || nonAlphaNumeric2)
        {
            // One point for non-alphanumeric.
            return 1;
        }
        return 0;
    }

    // Define some regex patterns for matching boundaries.
    // Regex BLANKLINEEND = new Regex("\\n\\r?\\n\\Z");
    // Regex BLANKLINESTART = new Regex("\\A\\r?\\n\\r?\\n");

    /**
     * Reduce the number of edits by eliminating operationally trivial
     * equalities.
     * @param diffs List of Diff objects.
     */
    void diff_cleanupEfficiency(vector<Diff> &diffs)
    {
        bool changes = false;
        // Stack of indices where equalities are found.
        stack<int> equalities;
        // Always equal to equalities[equalitiesLength-1][1]
        wstring lastEquality;
        int pointer = 0; // Index of current position.
        // Is there an insertion operation before the last equality.
        bool pre_ins = false;
        // Is there a deletion operation before the last equality.
        bool pre_del = false;
        // Is there an insertion operation after the last equality.
        bool post_ins = false;
        // Is there a deletion operation after the last equality.
        bool post_del = false;
        while (pointer < diffs.size())
        {
            if (diffs[pointer].operation == NORMAL)
            { // Equality found.
                if (diffs[pointer].text.size() < Diff_EditCost && (post_ins || post_del))
                {
                    // Candidate found.
                    equalities.push(pointer);
                    pre_ins = post_ins;
                    pre_del = post_del;
                    lastEquality = diffs[pointer].text;
                }
                else
                {
                    // Not a candidate, and can never become one.
                    while(equalities.size())
                    {
                        equalities.pop();
                    }

                    lastEquality.clear();
                }
                post_ins = post_del = false;
            }
            else
            { // An insertion or deletion.
                if (diffs[pointer].operation == DELETE)
                {
                    post_del = true;
                }
                else
                {
                    post_ins = true;
                }
                /*
           * Five types to be split:
           * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
           * <ins>A</ins>X<ins>C</ins><del>D</del>
           * <ins>A</ins><del>B</del>X<ins>C</ins>
           * <ins>A</del>X<ins>C</ins><del>D</del>
           * <ins>A</ins><del>B</del>X<del>C</del>
           */
                if ((lastEquality.size() != 0) && ((pre_ins && pre_del && post_ins && post_del) || ((lastEquality.size() < Diff_EditCost / 2) && ((pre_ins ? 1 : 0) + (pre_del ? 1 : 0) + (post_ins ? 1 : 0) + (post_del ? 1 : 0)) == 3)))
                {
                    // Duplicate record.
                    Diff tmp_diff(DELETE, lastEquality);
                    diffs.insert(diffs.begin() + equalities.top(), tmp_diff);
                    // Change second copy to insert.
                    diffs[equalities.top() + 1].operation = INSERT;
                    equalities.pop(); // Throw away the equality we just deleted.
                    lastEquality.clear();
                    if (pre_ins && pre_del)
                    {
                        // No changes made which could affect previous entry, keep going.
                        post_ins = post_del = true;
                        while(equalities.size())
                        {
                            equalities.pop();
                        }
                    }
                    else
                    {
                        if (equalities.size() > 0)
                        {
                            equalities.pop();
                        }

                        pointer = equalities.size() > 0 ? equalities.top() : -1;
                        post_ins = post_del = false;
                    }
                    changes = true;
                }
            }
            pointer++;
        }

        if (changes)
        {
            diff_cleanupMerge(diffs);
        }
    }


    bool startWith(const std::wstring& src, const std::wstring& prefix)
    {
        int m = src.size();
        int n = prefix.size();

        if(m <= 0 || n <= 0 || m < n)
            return false;
        
        for(int i =0; i < n; i++)
        {
            if(src[i] != prefix[i])
            {
                return false;
            }
        }

        return true;
    }


    bool endWith(const std::wstring& src, const std::wstring& postfix)
    {
        int m = src.size();
        int n = postfix.size();

        if(m <= 0 || n <= 0 || m < n)
            return false;
        
        for(int i = 0; i < n; i++)
        {
            if(src[m-n + i] != postfix[i])
            {
                return false;
            }
        }

        return true;
    }


    /**
     * Reorder and merge like edit sections.  Merge equalities.
     * Any edit section can move as long as it doesn't cross an equality.
     * @param diffs List of Diff objects.
     */
    void diff_cleanupMerge(vector<Diff>& diffs)
    {
        // Add a dummy entry at the end.
        diffs.emplace_back(NORMAL, wstring());
        int pointer = 0;
        int count_delete = 0;
        int count_insert = 0;
        wstring text_delete;
        wstring text_insert;
        int commonlength;
        while (pointer < diffs.size())
        {
            switch (diffs[pointer].operation)
            {
            case INSERT:
                count_insert++;
                text_insert += diffs[pointer].text;
                pointer++;
                break;
            case DELETE:
                count_delete++;
                text_delete += diffs[pointer].text;
                pointer++;
                break;
            case NORMAL:
                // Upon reaching an equality, check for prior redundancies.
                if (count_delete + count_insert > 1)
                {
                    if (count_delete != 0 && count_insert != 0)
                    {
                        // Factor out any common prefixies.
                        commonlength = diff_commonPrefix(text_insert, text_delete);
                        if (commonlength != 0)
                        {
                            if ((pointer - count_delete - count_insert) > 0 &&
                                diffs[pointer - count_delete - count_insert - 1].operation == NORMAL)
                            {
                                diffs[pointer - count_delete - count_insert - 1].text += text_insert.substr(0, commonlength);
                            }
                            else
                            {
                                diffs.insert(diffs.begin(), Diff(NORMAL, text_insert.substr(0, commonlength)));
                                pointer++;
                            }
                            text_insert = text_insert.substr(commonlength);
                            text_delete = text_delete.substr(commonlength);
                        }
                        // Factor out any common suffixies.
                        commonlength = diff_commonSuffix(text_insert, text_delete);
                        if (commonlength != 0)
                        {
                            diffs[pointer].text = text_insert.substr(text_insert.size() - commonlength) + diffs[pointer].text;
                            text_insert = text_insert.substr(0, text_insert.size() - commonlength);
                            text_delete = text_delete.substr(0, text_delete.size() - commonlength);
                        }
                    }
                    // Delete the offending records and add the merged ones.
                    pointer -= count_delete + count_insert;
                    diffs.erase(diffs.begin() + pointer, diffs.begin() + pointer + count_delete + count_insert);
                    if (text_delete.size() != 0)
                    {
                        diffs.insert(diffs.begin() + pointer, Diff(DELETE, text_delete));
                        pointer++;
                    }
                    if (text_insert.size() != 0)
                    {
                        diffs.insert(diffs.begin() + pointer, Diff(INSERT, text_insert));
                        pointer++;
                    }
                    pointer++;
                }
                else if (pointer != 0 && diffs[pointer - 1].operation == NORMAL)
                {
                    // Merge this equality with the previous one.
                    diffs[pointer - 1].text += diffs[pointer].text;
                    diffs.erase(diffs.begin() + pointer);
                }
                else
                {
                    pointer++;
                }
                count_insert = 0;
                count_delete = 0;
                text_delete.clear();
                text_insert.clear();
                break;
            }
        }

        if (diffs.back().text.empty())
        {
            diffs.pop_back(); // Remove the dummy entry at the end.
        }

        // Second pass: look for single edits surrounded on both sides by
        // equalities which can be shifted sideways to eliminate an equality.
        // e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
        bool changes = false;
        pointer = 1;
        // Intentionally ignore the first and last element (don't need checking).
        while (pointer < (diffs.size() - 1))
        {
            if (diffs[pointer - 1].operation == NORMAL && diffs[pointer + 1].operation == NORMAL)
            {
                // This is a single edit surrounded by equalities.
                if (endWith(diffs[pointer].text, diffs[pointer - 1].text))
                {
                    // Shift the edit over the previous equality.
                    diffs[pointer].text = diffs[pointer - 1].text +
                                          diffs[pointer].text.substr(0, diffs[pointer].text.size() -
                                                                               diffs[pointer - 1].text.size());
                    diffs[pointer + 1].text = diffs[pointer - 1].text + diffs[pointer + 1].text;
                    diffs.erase(diffs.begin() + pointer - 1);
                    changes = true;
                }
                else if (startWith(diffs[pointer].text, diffs[pointer + 1].text))
                {
                    // Shift the edit over the next equality.
                    diffs[pointer - 1].text += diffs[pointer + 1].text;
                    diffs[pointer].text = diffs[pointer].text.substr(diffs[pointer + 1].text.size()) + diffs[pointer + 1].text;
                    diffs.erase(diffs.begin() + pointer + 1);
                    changes = true;
                }
            }
            pointer++;
        }
        // If shifts were made, the diff needs reordering and another shift sweep.
        if (changes)
        {
            diff_cleanupMerge(diffs);
        }
    }

    // /**
    //  * loc is a location in text1, compute and return the equivalent location in
    //  * text2.
    //  * e.g. "The cat" vs "The big cat", 1->1, 5->8
    //  * @param diffs List of Diff objects.
    //  * @param loc Location within text1.
    //  * @return Location within text2.
    //  */
    // int diff_xIndex(list<Diff> diffs, int loc) {
    //   int chars1 = 0;
    //   int chars2 = 0;
    //   int last_chars1 = 0;
    //   int last_chars2 = 0;
    //   Diff lastDiff = null;
    //   foreach (Diff aDiff in diffs) {
    //     if (aDiff.operation != Operation.INSERT) {
    //       // Equality or deletion.
    //       chars1 += aDiff.text.Length;
    //     }
    //     if (aDiff.operation != Operation.DELETE) {
    //       // Equality or insertion.
    //       chars2 += aDiff.text.Length;
    //     }
    //     if (chars1 > loc) {
    //       // Overshot the location.
    //       lastDiff = aDiff;
    //       break;
    //     }
    //     last_chars1 = chars1;
    //     last_chars2 = chars2;
    //   }
    //   if (lastDiff != null && lastDiff.operation == Operation.DELETE) {
    //     // The location was deleted.
    //     return last_chars2;
    //   }
    //   // Add the remaining character length.
    //   return last_chars2 + (loc - last_chars1);
    // }

    /**
     * Compute the Levenshtein distance; the number of inserted, deleted or
     * substituted characters.
     * @param diffs List of Diff objects.
     * @return Number of changes.
     */
    int diff_levenshtein(list<Diff> &diffs)
    {
        int levenshtein = 0;
        int insertions = 0;
        int deletions = 0;
        for(auto &aDiff : diffs)
        {
            switch (aDiff.operation)
            {
            case INSERT:
                insertions += aDiff.text.size();
                break;
            case DELETE:
                deletions += aDiff.text.size();
                break;
            case NORMAL:
                // A deletion and an insertion is one substitution.
                levenshtein += std::max(insertions, deletions);
                insertions = 0;
                deletions = 0;
                break;
            }
        }
        levenshtein += std::max(insertions, deletions);
        return levenshtein;
    }

    /**
     * Find the differences between two texts.  Simplifies the problem by
     * stripping any common prefix or suffix off the texts before diffing.
     * @param text1 Old string to be diffed.
     * @param text2 New string to be diffed.
     * @param checklines Speedup flag.  If false, then don't run a
     *     line-level diff first to identify the changed areas.
     *     If true, then run a faster slightly less optimal diff.
     * @param deadline Time when the diff should be complete by.  Used
     *     internally for recursive calls.  Users should set DiffTimeout
     *     instead.
     * @return List of Diff objects.
     */
    vector<Diff> diff_main(std::wstring text1, std::wstring text2, bool checklines, clock_t deadline)
    {
        // Check for null inputs not needed since null can't be passed in C#.

        // Check for equality (speedup).
        vector<Diff> diffs;
        if (text1 == text2)
        {
            if (text1.size())
            {
                diffs.emplace_back(NORMAL, text1);
            }
            return diffs;
        }

        // Trim off common prefix (speedup).
        int commonlength = diff_commonPrefix(text1, text2);
        wstring commonprefix = text1.substr(0, commonlength);
        text1 = text1.substr(commonlength);
        text2 = text2.substr(commonlength);

        // Trim off common suffix (speedup).
        commonlength = diff_commonSuffix(text1, text2);
        wstring commonsuffix = text1.substr(text1.size() - commonlength);
        text1 = text1.substr(0, text1.size() - commonlength);
        text2 = text2.substr(0, text2.size() - commonlength);

        // Compute the diff on the middle block.
        diffs = diff_compute(text1, text2, checklines, deadline);

        // Restore the prefix and suffix.
        if (commonprefix.size() != 0)
        {
            diffs.insert(diffs.begin(), Diff(NORMAL, commonprefix));
        }
        if (commonsuffix.size() != 0)
        {
            diffs.emplace_back(NORMAL, commonsuffix);
        }

        diff_cleanupMerge(diffs);
        return diffs;
    }

    /**
     * Find the differences between two texts.
     * @param text1 Old string to be diffed.
     * @param text2 New string to be diffed.
     * @param checklines Speedup flag.  If false, then don't run a
     *     line-level diff first to identify the changed areas.
     *     If true, then run a faster slightly less optimal diff.
     * @return List of Diff objects.
     */
    vector<Diff> diff_main(std::wstring text1, std::wstring text2, bool checklines)
    {
        // Set a deadline by which time the diff must be complete.
        clock_t deadline;
        if (Diff_Timeout <= 0)
        {
            deadline = std::numeric_limits<clock_t>::max();
        }
        else
        {
            deadline = clock() + (clock_t)(Diff_Timeout * CLOCKS_PER_SEC);
        }

        return diff_main(text1, text2, checklines, deadline);
    }

    /**
     * Find the differences between two texts.
     * Run a faster, slightly less optimal diff.
     * This method allows the 'checklines' of diff_main() to be optional.
     * Most of the time checklines is wanted, so default to true.
     * @param text1 Old string to be diffed.
     * @param text2 New string to be diffed.
     * @return List of Diff objects.
     */
    vector<Diff> diff_main(wstring text1, wstring text2)
    {
        vector<Diff> diffs = diff_main(text1, text2, false);
        diff_cleanupSemantic(diffs);
        return diffs;
    }



std::vector<XChar> extract_result(std::vector<XChar>& chars, int offset, int length, CharStatus status, std::vector<int>& origin_text_index, std::wstring& origin_text)
{
    std::vector<XChar> result_list;

    if(length == 0)
    {
        XChar ch;

        if(chars.size())
        {
            if(offset > 0)
            {
                ch.box = chars[offset-1].box;
                ch.box.left = ch.box.right;
                ch.page_index = chars[offset-1].page_index;
                ch.line_index = chars[offset-1].line_index;
            }
            else
            {
                ch.box = chars[offset].box;
                ch.box.right = ch.box.left;
                ch.page_index = chars[offset].page_index;
                ch.line_index = chars[offset].line_index;
            }
        }
        else
        {
            ch.box.assign(1, 1,1, 1);
            ch.page_index = 0;
            ch.line_index = 0;
        }

        
        ch.char_status = status;
        result_list.push_back(ch);
        return result_list;
    }

    XChar prev_char = chars[offset];
    XChar comb_char = prev_char;

    int start_index = offset; // 偏移量
    for(int i = offset+1; i < offset + length; i++)
    {
        if(prev_char.can_merge(chars[i]))
        {
            comb_char.merge(chars[i]);
            prev_char = chars[i];
        }
        else
        {
            comb_char.char_status = status;
            if(origin_text_index.size() == chars.size())
            {
                comb_char.text = origin_text.substr(origin_text_index[start_index], origin_text_index[start_index+comb_char.pos_list.size()-1] - origin_text_index[start_index] + 1); // 映射回过滤标点前的文本内容
            }
            result_list.push_back(comb_char);
            prev_char = chars[i];
            comb_char = prev_char;
            start_index = i; 
        }
    }
    comb_char.char_status = status;
    if(origin_text_index.size() == chars.size())
    {
        comb_char.text = origin_text.substr(origin_text_index[start_index], origin_text_index[start_index+comb_char.pos_list.size()-1] - origin_text_index[start_index] + 1); // 映射回过滤标点前的文本内容
    }
    result_list.push_back(comb_char);
    return result_list;
}


int diff_main_myers(const std::wstring& para1, 
                    const std::wstring& para2, 
                    std::vector<XChar>& char_x, 
                    std::vector<XChar>& char_y,
                    std::vector<std::vector<XChar> >& return_list1, 
                    std::vector<std::vector<XChar> >& return_list2, 
                    CharCounter& char_counter, std::vector<int>& origin_text_index1, std::vector<int>& origin_text_index2, std::wstring& origin_text1, std::wstring& origin_text2,
                    bool merge_diff)
{
    st_merge_diff = merge_diff;
    vector<Diff> diffs = diff_main(para1, para2);
    diffs.emplace_back(NORMAL, wstring());

    int pointer = 0;
    int count_delete = 0;
    int count_insert = 0;
    int offset_delete = 0;
    int offset_insert = 0;
    int length_delete = 0;
    int length_insert = 0;
    
    while (pointer < diffs.size())
    {
        switch (diffs[pointer].operation)
        {
        case INSERT:
            count_insert++;
            length_insert += diffs[pointer].text.size();
            pointer++;
            break;
        case DELETE:
            count_delete++;
            length_delete += diffs[pointer].text.size();
            pointer++;
            break;
        case NORMAL:
            // Upon reaching an equality, check for prior redundancies.
            if (count_delete + count_insert > 0)
            {
                CharStatus status;
                if (count_delete != 0 && count_insert != 0)
                {
                    status = CHANGE;
                }
                else if(count_delete != 0)
                {
                    status = DELETE;
                }
                else
                {
                    status = INSERT;
                }

                vector<XChar> chars_delete = extract_result(char_x, offset_delete, length_delete, status, origin_text_index1, origin_text1);
                vector<XChar> chars_insert = extract_result(char_y, offset_insert, length_insert, status, origin_text_index2, origin_text2);
                return_list1.push_back(chars_delete);
                return_list2.push_back(chars_insert);
            }
            
            offset_delete += length_delete;
            offset_insert += length_insert;

            if(diffs[pointer].text.size())
            {
                vector<XChar> chars_delete = extract_result(char_x, offset_delete, diffs[pointer].text.size(), NORMAL, origin_text_index1, origin_text1);
                vector<XChar> chars_insert = extract_result(char_y, offset_insert, diffs[pointer].text.size(), NORMAL, origin_text_index2, origin_text2);
                return_list1.push_back(chars_delete);
                return_list2.push_back(chars_insert);
            }

            count_insert = 0;
            count_delete = 0;

            char_counter.update(length_delete+diffs[pointer].text.size(), length_insert + diffs[pointer].text.size(), diffs[pointer].text.size());

            offset_delete += diffs[pointer].text.size();
            offset_insert += diffs[pointer].text.size();
            length_delete = 0;
            length_insert = 0;
            pointer++;
            break;
        }
    }
    return 0;
}


} // namespace xwb