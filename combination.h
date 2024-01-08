#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <set>
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>

namespace Combination
{
    using namespace std;
    using namespace indicators;
    typedef map<string, vector<string>> CombinationMap;
    typedef vector<vector<string>> CombinationArray;
    typedef vector<int> Odometer;

    auto form_combination(Odometer odometer, CombinationArray combinations, vector<vector<string>> &output)
    {
        vector<string> c;
        for (int i = 0; i < odometer.size(); i++)
        {
            auto it = combinations.begin();
            std::advance(it, i);
            c.push_back(combinations.at(i).at(odometer[i]));
        }

        output.push_back(c);
    }
    bool odometer_increment(Odometer &odometer, CombinationArray combinations)
    {
        for (int i = odometer.size() - 1; i >= 0; i--)
        {
            auto maxee = combinations[i].size() - 1;
            if (odometer[i] + 1 <= maxee)
            {
                odometer[i]++;
                return true;
            }
            else
            {
                if (i - 1 < 0)
                {
                    return false;
                }
                else
                {
                    odometer[i] = 0;
                    continue;
                }
            }
        }
    }

    Combination::CombinationArray combine_vectors(string url, CombinationMap combinations)
    {
        vector<vector<string>> result;
        Odometer odometer;
        int total = 1;
        for (int i = 0; i < combinations.size(); i++)
        {
            odometer.push_back(0);
        }
        CombinationArray arr;
        for (auto const &[key, val] : combinations)
        {
            arr.push_back(val);
            total *= val.size();
        }
        reverse(arr.begin(), arr.end());

        form_combination(odometer, arr, result);

        int counter = 1;
        show_console_cursor(false);
        indicators::ProgressBar p{
            option::BarWidth{50},
            option::Start{"["},
            option::Fill{"■"},
            option::Lead{"■"},
            option::Remainder{" "},
            option::End{"]"},
            option::ForegroundColor{indicators::Color::green},
            option::PrefixText{"generating all possible urls"},
            option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}};
        while (odometer_increment(odometer, arr))
        {
            auto o = odometer;
            // pool.enqueue([&counter, o, arr, &result, url, total, &m]
            //              {
            counter++;
            // cout << '\r' << "generating possible urls for \"" << url << "\": " << counter << " / " << total;
            // cout << '\r' << counter << "/" << total;
            auto progress = counter / (float)total * 100;
            stringstream s;
            s.precision(3);
            s<<progress;
            p.set_option(option::PostfixText{to_string(counter) + "/" + to_string(total) + " " + s.str()+"%"});
            p.set_progress(progress);
            form_combination(o, arr, result); 
            // });
        }
        p.mark_as_completed();
        show_console_cursor(true);

        return result;
    }
} // namespace Counter
