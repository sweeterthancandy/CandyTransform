#include "CandyTransform/Transform.h"
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

int main(){
        using namespace CandyTransform;
        struct PushFold : Transform<std::string, std::string>{
                virtual void Apply(TransformControl* ctrl, ParamType in)override{
                        if( in == "ff" )
                                return;
                        if( in.size() == 3 )
                                return;
                        ctrl->Emit( in + "p");
                        ctrl->Emit( in + "f");
                        ctrl->Next(std::make_shared<PushFold>());
                }
        };
        TransformContext<std::string> ctx;
        auto path = ctx.Start();
        path->Next(std::make_shared<PushFold>());
        for(auto const& result : ctx(std::string{})){
                std::cout << "result => " << result << "\n"; // __CandyPrint__(cxx-print-scalar,result)
        }
}
