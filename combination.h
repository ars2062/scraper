#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <set>
#include "pool.h"

namespace Combination
{
    using namespace std;
    typedef map<string, vector<string>> CombinationMap;
    typedef vector<vector<string>> CombinationArray;
    typedef vector<int> Odometer;

    auto form_combination(Odometer odometer, CombinationArray combinations, vector<vector<string>> &output, mutex &m)
    {
        vector<string> c;
        for (int i = 0; i < odometer.size(); i++)
        {
            auto it = combinations.begin();
            std::advance(it, i);
            c.push_back(combinations.at(i).at(odometer[i]));
        }

        m.lock();
        output.push_back(c);
        m.unlock();
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

    Combination::CombinationArray combine_vectors(string url, CombinationMap combinations, ThreadPool &pool)
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

        mutex m;
        form_combination(odometer, arr, result, m);

        int counter = 1;

        while (odometer_increment(odometer, arr))
        {
            auto o = odometer;
            pool.enqueue([&counter, o, arr, &result, url, total, &m]
                         {
            counter++;
            // cout << '\r' << "generating possible urls for \"" << url << "\": " << counter << " / " << total;
            cout << '\r' << counter << "/" << total;
            form_combination(o, arr, result, m); });
        }

        cout << endl;

        return result;
    }
} // namespace Counter
