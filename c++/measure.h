// measure.h : includes measurement templates.
// by: allison morris

#include <iostream>
#include <iomanip>
#include "clocker.h"

// template executor that also prints a message.
template <class Predicate, class Data> void measure(const std::string& txt, Predicate pred, Data d, clocker::mode mode, int count) {
    if (txt.length() > 60) { txt = txt.substr(0, 60); }
    std::cout << std::setw(60) << std::setfill(' ') << txt;
    
    clock_runner<Pred, Data> runner(pred, d);
    long time = runner.run(count, mode);
    
    std::cout << time << std::endl;
}

// template for running and measuring tests.
template <class Predicate, class Data> class clock_runner {
public:
    
    clock_runner() : _pred(), _data() { }
    
    clock_runner(const Predicate& p, const Data& d) : _pred(p), _data(d) { }
    
    long run(int count, clocker::mode m) {
        clocker clk(m);
        
        clk.begin();
        for (int i = 0; i < count; ++i) {
            _pred.run(d);
        }
        clk.end();
        
        return clk.difference() / count;
    }
    
private:
    Predicate _pred;
    Data _data;
};