#include "CandyTransform/Transform.h"
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

int main(){
        using namespace CandyTransform;
        struct QuoteOne : Transform<std::string, std::string>{
                QuoteOne(){
                        SetName("QuoteOne");
                }
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        auto copy = in;
                        copy[0] = '_';
                        ctrl->Emit(copy);
                }
        };
        struct TimesTwo : Transform<std::string, std::string>{
                TimesTwo(){
                        SetName("TimesTwo");
                }
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        auto ret = in + in;
                        ctrl->Emit(ret);
                        auto dp = ctrl->DeclPath();
                        if( ret.size() && ret[0] == '1' ){
                                dp->Next(std::make_shared<QuoteOne>());
                        }
                }
        };
        struct ToString : Transform<int, std::string>{
                ToString(){
                        SetName("ToString");
                }
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        ctrl->Emit( boost::lexical_cast<std::string>(in) );
                }
        };
        struct AllPerms : Transform<std::string, std::string>{ 
                AllPerms(){
                        SetName("AllPerms");
                }
                virtual void Apply(TransformControl* ctrl, ParamType in){
                        auto s = in;
                        std::sort(s.begin(), s.end());
                        do{
                                ctrl->Emit( s );
                        }while(std::next_permutation(s.begin(), s.end()));
                }
        };
        struct MaybeStop : Transform<std::string, std::string>{ 
                MaybeStop(){
                        SetName("MaybeStop");
                }
                virtual void Apply(TransformControl* ctrl, ParamType in){
                        if( in[0] != '2' ){
                                ctrl->Continue();
                        }
                }
        };
        struct Print : Transform<std::string, std::string>{
                Print(){
                        SetName("Print");
                }
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        std::cout << "in => " << in << "\n"; // __CandyPrint__(cxx-print-scalar,in)
                }
        };
        TransformContext ctx;
        auto path = ctx.Start();
        path->Next(std::make_shared<ToString>())
            ->Next(std::make_shared<AllPerms>())
            ->Next(std::make_shared<MaybeStop>())
            ->Next(std::make_shared<TimesTwo>())
        ;
        for(auto const& result : ctx.Execute<std::string>(int{241}) ){
                std::cout << "result => " << result << "\n"; // __CandyPrint__(cxx-print-scalar,result)
        }
}
