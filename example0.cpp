#include "CandyTransform/Transform.h"
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

int main(){
        using namespace CandyTransform;
        struct QuoteOne : Transform<std::string, std::string>{
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        in[0] = '_';
                }
        };
        struct TimesTwo : Transform<std::string, std::string>{
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        auto ret = in + in;
                        ctrl->Emit(ret);
                        if( ret.size() && ret[0] == '1' ){
                                ctrl->Next(std::make_shared<QuoteOne>());
                        }
                        
                }
        };
        struct ToString : Transform<int, std::string>{
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        ctrl->Emit( boost::lexical_cast<std::string>(in) );
                }
        };
        struct AllPerms : Transform<std::string, std::string>{ 
                virtual void Apply(TransformControl* ctrl, ParamType in){
                        auto s = in;
                        std::sort(s.begin(), s.end());
                        do{
                                ctrl->Emit( s );
                        }while(std::next_permutation(s.begin(), s.end()));
                }
        };
        struct Print : Transform<std::string, std::string>{
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        std::cout << "in => " << in << "\n"; // __CandyPrint__(cxx-print-scalar,in)
                }
        };
        TransformContext<std::string> ctx;
        auto path = ctx.Start();
        path->Next(std::make_shared<ToString>());
        path->Next(std::make_shared<AllPerms>());
        path->Next(std::make_shared<TimesTwo>());
        for(auto const& result : ctx(int{241}) ){
                std::cout << "result => " << result << "\n"; // __CandyPrint__(cxx-print-scalar,result)
        }
}
