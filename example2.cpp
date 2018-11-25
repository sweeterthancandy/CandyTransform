
#include "CandyTransform/Transform.h"
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

/*
        Given a prime, what is it's factorization
 */
namespace {
        using namespace CandyTransform;


        struct Factorization{
                friend std::ostream& operator<<(std::ostream& ostr,         Factorization const& self){
                        typedef std::vector<size_t>::const_iterator CI0;
                        const char* comma = "";
                        ostr << "numbers" << " = {";
                        for(CI0 iter= self.numbers.begin(), end=self.numbers.end();iter!=end;++iter){
                                ostr << comma << *iter;
                                comma = ", ";
                        }
                        ostr << "}";
                        typedef std::vector<std::string>::const_iterator CI1;
                        comma = "";
                        ostr << "tokens" << " = {";
                        for(CI1 iter= self.tokens.begin(), end=self.tokens.end();iter!=end;++iter){
                                ostr << comma << *iter;
                                comma = ", ";
                        }
                        ostr << "}";
                        ostr << ", target = " << self.target;
                        return ostr;
                }
                std::vector<size_t> numbers;
                std::vector<std::string> tokens;
                size_t target;
        };

        struct Operator{
                virtual ~Operator()=default;
                virtual void Emit(std::function<void(Factorization const&)> continuation, Factorization const& value)const=0;
        };
        enum OperatorArity{
                AR_Unary,
                AR_CommutativeBinary,
                AR_NonCommutativeBinary,
        };
        struct NonTerminalOperator : Operator{
                explicit NonTerminalOperator(OperatorArity arity)
                        :arity_(arity)
                {}
                virtual void Emit(std::function<void(Factorization const&)> continuation, Factorization const& value)const{
                        std::vector<size_t> p(value.numbers.size());
                        switch(arity_){
                                case AR_Unary:
                                {
                                        std::vector<size_t> v(1);
                                        for(size_t idx=0;idx!=value.numbers.size();++idx){
                                                v[0] = idx;
                                                EmitPerm(continuation, value, v);
                                        }
                                        break;
                                }
                                case AR_CommutativeBinary:
                                case AR_NonCommutativeBinary:
                                {
                                        std::vector<size_t> p(value.numbers.size());
                                        p[0] = 1;
                                        p[1] = 1;
                                        std::sort(p.begin(), p.end());
                                        std::vector<size_t> v(2);
                                        do{
                                                size_t* out = &v[0];
                                                for(size_t idx=0;idx!=value.numbers.size();++idx){
                                                        if( p[idx] ){
                                                                *out = idx;
                                                                ++out;
                                                                continue;
                                                        }
                                                }

                                                EmitPerm(continuation, value, v);
                                                if( arity_ == AR_NonCommutativeBinary ){
                                                        std::swap(v[0], v[1]);
                                                        EmitPerm(continuation, value, v);
                                                }
                                        }while(std::next_permutation(p.begin(), p.end()));

                                        break;
                                }
                        }
                }
                virtual void EmitPerm(std::function<void(Factorization const&)> continuation, Factorization const& value, std::vector<size_t> const perm)const=0;
        protected:
                static Factorization CopyExPerm(Factorization const& value, std::vector<size_t> const perm){
                        Factorization result;
                        result.target = value.target;
                        for(size_t idx=0;idx!=value.numbers.size();++idx){
                                if( perm[0] == idx || perm[1] == idx )
                                        continue;
                                result.numbers.push_back(value.numbers[idx]);
                                result.tokens.push_back(value.tokens[idx]);
                        }
                        return result;
                }
        private:
                OperatorArity arity_;
        };

        struct AddOperator : NonTerminalOperator{
                AddOperator():NonTerminalOperator(AR_CommutativeBinary){}
                virtual void EmitPerm(std::function<void(Factorization const&)> continuation, Factorization const& value, std::vector<size_t> const perm)const{
                        auto next = CopyExPerm(value, perm);
                        auto a = value.numbers[perm[0]];
                        auto a_s = value.tokens[perm[0]];
                        auto b = value.numbers[perm[1]];
                        auto b_s = value.tokens[perm[1]];
                        next.numbers.push_back( a + b);
                        next.tokens.push_back( "(" + a_s + "+" + b_s + ")");
                        continuation(next);
                }
        };
        struct MulOperator : NonTerminalOperator{
                MulOperator():NonTerminalOperator(AR_CommutativeBinary){}
                virtual void EmitPerm(std::function<void(Factorization const&)> continuation, Factorization const& value, std::vector<size_t> const perm)const{
                        auto next = CopyExPerm(value, perm);
                        auto a = value.numbers[perm[0]];
                        auto a_s = value.tokens[perm[0]];
                        auto b = value.numbers[perm[1]];
                        auto b_s = value.tokens[perm[1]];
                        next.numbers.push_back( a * b);
                        next.tokens.push_back( "("+ a_s + "*" + b_s + ")");
                        continuation(next);
                }
        };


        struct F : Transform<Factorization, Factorization>{
                std::vector<std::shared_ptr<Operator> > ops_;
                F(){
                        ops_.push_back(std::make_shared<AddOperator>());
                        ops_.push_back(std::make_shared<MulOperator>());
                }
                virtual void Apply(TransformControl* ctrl, ParamType in)override{

                        if( in.numbers.size() == 1){
                                if(in.numbers.back() == in.target ){
                                        std::cout << "in.numbers.back() => " << in.numbers.back() << "\n"; // __CandyPrint__(cxx-print-scalar,in.numbers.back())
                                        ctrl->Return(in.tokens.back());
                                }
                                return;
                        }

                        for(auto op : ops_ ){
                                op->Emit( [&](auto&& f){ ctrl->Emit(f); }, in);
                        }
                        
                        #if 0
                        std::vector<size_t> p(in.numbers.size());
                        p[0] = 1;
                        p[1] = 1;
                        std::sort(p.begin(), p.end());
                        do{
                                Factorization proto;
                                proto.target = in.target;
                                std::vector<size_t> v;
                                for(size_t idx=0;idx!=in.numbers.size();++idx){
                                        if( p[idx] ){
                                                v.push_back(idx);
                                                continue;
                                        }
                                        proto.numbers.push_back( in.numbers[idx] );
                                        proto.tokens.push_back( in.tokens[idx] );
                                }

                                auto a = in.numbers[v[0]];
                                auto a_s = in.tokens[v[0]];
                                auto b = in.numbers[v[1]];
                                auto b_s = in.tokens[v[1]];

                                Factorization add = proto;
                                add.numbers.push_back( a + b);
                                add.tokens.push_back( "(" + a_s + "+" + b_s + ")");
                                ctrl->Emit(add);

                                Factorization mul = proto;
                                mul.numbers.push_back( a * b );
                                mul.tokens.push_back( "(" + a_s + "*" +  b_s + ")");
                                ctrl->Emit(mul);
                                
                                if( b != 0 ){
                                        Factorization lmod = proto;
                                        lmod.numbers.push_back(  a % b );
                                        lmod.tokens.push_back( "(" + a_s + "*" +  b_s + ")");
                                        ctrl->Emit(lmod);
                                }
                                if( a != 0 ){
                                        Factorization rmod = proto;
                                        rmod.numbers.push_back(  b % a );
                                        rmod.tokens.push_back( "(" + b_s + "*" +  a_s + ")");
                                        ctrl->Emit(rmod);
                                }



                        }while(std::next_permutation(p.begin(), p.end()));
                        #endif

                        ctrl->DeclPath()->Next(std::make_shared<F>());
                }
        };
} // end namespace anon

int main(){

        TransformContext ctx;

        auto path = ctx.Start();
        path->Next(std::make_shared<F>());

        Factorization init;
        
        /*
         
           2 + 4 = 6

         */
        #if 0
        init.numbers = std::vector<size_t>{2,4};
        init.target = 6;
        #endif



        /*
         
          ( 2 * (3 + 4) ) * 5 = 70

         */
        #if 1
        init.numbers = std::vector<size_t>{3,4,2,5};
        init.target = 70;
        #endif
        
        
        /*
         
          ( ( 19*97 % 2 ) * (3 + 4) ) * 5 = 35

         */
        #if 0
        init.numbers = std::vector<size_t>{3,4,19,5,2};
        init.target = 35;
        #endif
        for(auto _ : init.numbers ){
                init.tokens.push_back(boost::lexical_cast<std::string>(_));
        }

        for(auto const& result : ctx.Execute<std::string>(init) ){
                std::cout << "result => " << result << "\n"; // __CandyPrint__(cxx-print-scalar,result)
        }
}
