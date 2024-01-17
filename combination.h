#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <set>
#include <future>
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>

namespace Combination {
    using namespace std;
    using namespace indicators;
    typedef map<string, vector<string>> CombinationMap;
    typedef vector<vector<string>> CombinationArray;
    typedef vector<int> Odometer;

    void form_combination(Odometer odometer, CombinationArray combinations, vector<vector<string>> &output) {
        vector<string> c;
        c.reserve(odometer.size());
        for (int i = 0; i < odometer.size(); i++) {
//            auto it = combinations.begin();
//            std::advance(it, i);
//            c.push_back(combinations.at(i).at(odometer[i]));
            c.push_back(combinations[i][odometer[i]]);
        }

        output.push_back(c);
    }

    bool odometer_increment(Odometer &odometer, const CombinationArray &combinations) {
        for (int i = odometer.size() - 1; i >= 0; i--) {
            auto max = combinations[i].size() - 1;
            if (odometer[i] + 1 <= max) {
                odometer[i]++;
                return true;
            } else {
                if (i - 1 < 0) {
                    return false;
                } else {
                    odometer[i] = 0;
                    continue;
                }
            }
        }
        return false;
    }

    CombinationArray combine_vectors(const CombinationMap &combinations) {
        vector<vector<string>> result;
        Odometer odometer;
        int total = 1;
        CombinationArray arr;
        for (const auto& [key, val] : combinations) {
            odometer.push_back(0);
            total *= val.size();
            arr.push_back(val);
        }

        reverse(arr.begin(), arr.end());

        form_combination(odometer, arr, result);

        int counter = 1;

        cout << "generating all possible URLs" << endl;

        while (odometer_increment(odometer, arr)) {
            counter++;
            form_combination(odometer, arr, result);
        }

        cout << "generated " << result.size() << " possibilities" << endl;

        return result;
    }
} // namespace Combination
