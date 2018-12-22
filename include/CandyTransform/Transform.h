#ifndef CANDY_TRANSFORM_H
#define CANDY_TRANSFORM_H

#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <list>
#include <unordered_map>
#include <queue>

#include <boost/lexical_cast.hpp>
#include <boost/type_index.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/typeid_of.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/optional.hpp>


namespace CandyTransform{


        struct GNode;

        struct GEdge{
                GEdge(GNode* from, GNode* to)
                        :from_(from),
                        to_(to)
                {}
                GNode* From()const{ return from_; }
                GNode* To()const{ return to_; }

                friend std::ostream& operator<<(std::ostream& ostr, GEdge const& e);

        private:
                friend struct Graph;
                GNode* from_;
                GNode* to_;
        };

        struct GNode{
                explicit GNode(std::string const& name)
                        :name_(name)
                {}


                std::vector<GEdge const*> EdgePath()const{
                        std::vector<GEdge const*> rpath;
                        GNode const* head = this;
                        for(;;){
                                // assume tree
                                if( head->in_.size() != 1 ){
                                        break;
                                }
                                auto e = head->in_.back();
                                rpath.push_back(head->in_.back());
                                head = rpath.back()->From();
                        }
                        return std::vector<GEdge const*>(rpath.rbegin(), rpath.rend());
                }

                std::string const& Name()const{ return name_; }
                
                std::vector<GNode*> TerminalNodes(){
                        std::vector<GNode*> terminals;
                        std::vector<GNode*> stack{this};
                        for(;stack.size();){
                                auto head = stack.back();
                                stack.pop_back();
                                if( head->IsTerminal() ){
                                        terminals.push_back(head);
                                } else{
                                        for(auto e : head->out_){
                                                stack.push_back(e->To());
                                        }
                                }
                        }
                        return terminals;
                }
                bool IsTerminal()const{ return out_.empty(); }
        

                friend std::ostream& operator<<(std::ostream& ostr, GNode const& self){
                        ostr << "{name=" << self.name_ << "}";
                        return ostr;
                }

                auto const& OutEdges(){ return out_; }

        private:
                friend struct Graph;
                std::string name_;
                std::vector<GEdge*> in_;
                std::vector<GEdge*> out_;
        };

        std::ostream& operator<<(std::ostream& ostr, GEdge const& e){
                return ostr << "{from=" << e.from_->Name() << ", to=" << e.to_->Name() << "}";
        }

        struct Graph{
                GNode* Node(std::string const& name_){
                        N.push_back(std::make_shared<GNode>(name_));
                        return N.back().get();
                }
                GEdge* Edge(GNode* a, GNode* b){
                        auto e = std::make_shared<GEdge>(a, b);
                        a->out_.push_back(e.get());
                        b->in_ .push_back(e.get());
                        E.push_back(e);
                        return E.back().get();
                }
        private:
                std::vector<std::shared_ptr<GNode> > N;
                std::vector<std::shared_ptr<GEdge> > E;
        };

        template<class T>
        struct GraphColouring : std::unordered_map<void const*, T>{
                T const& Color(void const* e)const{
                        auto iter = this->find(e);
                        if( iter == this->end())
                                BOOST_THROW_EXCEPTION(std::domain_error("no colour"));
                        return iter->second;
                }
        };







        struct TransformControl;

        namespace te = boost::type_erasure;
        namespace mpl = boost::mpl;

        using AnyType = te::any<
            mpl::vector<
                te::typeid_<>,
                //te::ostreamable<>,
                te::copy_constructible<>,
                te::assignable<>,
                // so that it can hold nothing
                te::relaxed 
            >
        >;

        /*
         * Transforms are to be typed, so this just earses that, and does run time
         * type checking for sugar
         */
        struct TransformBase{
                virtual ~TransformBase()=default;
                virtual void ApplyImpl(TransformControl* ctrl)=0;
                std::string const& Name()const{ return name_; }
        protected:
                void SetName(std::string const& name){
                        name_ = name;
                }
        protected:
                virtual boost::typeindex::type_index GetInType()const=0;
                virtual boost::typeindex::type_index GetOutType()const=0;
                std::string name_;
        };

        struct PathDecl{
                virtual ~PathDecl()=default;
                /*
                 * Add continuation
                 */
                virtual std::shared_ptr<PathDecl> Next(std::shared_ptr<TransformBase> ptr)=0;
        };

        /*
         * This is what is passed to transform
         */
        struct TransformControl{
                virtual ~TransformControl()=default;
                /*
                 *  Emit, as in the continuation programming model
                 *
                 *  template<class C>
                 *  void sqrt(C&c, double x){
                 *      c( +std::sqrt(x) );
                 *      c( -std::sqrt(x) );
                 *  }
                 *
                 *  template<class C>
                 *  void for_each(C& c, std::vector<std::string const>& v){
                 *      for(auto const& _ : v){
                 *          c(_);
                 *      }
                 *  }
                 */
                virtual void Emit(AnyType const& val)=0;
                /*
                 * End this path
                 */
                virtual void Error(std::string const& msg)=0;
                /*
                 * Retreive an argument
                 */
                virtual AnyType& Arg(size_t idx=0)=0;
                /*
                 * find out how deep we are in the path
                 */
                virtual size_t Depth()const=0;

                virtual std::shared_ptr<PathDecl> DeclPath()=0;

                // emit in arguments, ie
                //
                //    template<class C, class T>
                //    void f(C& c, T&& t){
                //        sideeffect();
                //        c(t);
                //    }
                virtual void Pass()=0;
                
                virtual void Return(AnyType const& value)=0;
                
                //virtual void Loop()=0;
        };

        template<class In_,class Out_>
        struct Transform : TransformBase{
                using ParamType = In_&;
                virtual void Apply(TransformControl* ctrl, ParamType in)=0;

        protected:
                virtual void ApplyImpl(TransformControl* ctrl)override{

                        auto& arg0 = ctrl->Arg(0);
                        if( te::typeid_of(arg0) != GetInType()){
                                std::stringstream sstr;
                                sstr << "Bad cast, expected " << GetInType().pretty_name() << ", but got " << 
                                        boost::typeindex::type_index(te::typeid_of(arg0)).pretty_name();
                                ctrl->Error(sstr.str());
                                return;
                        }
                        this->Apply( ctrl, te::any_cast<ParamType>(ctrl->Arg(0)) );
                }
                
                virtual boost::typeindex::type_index GetInType()const{
                        return boost::typeindex::type_id<In_>().type_info();
                }
                virtual boost::typeindex::type_index GetOutType()const{
                        return boost::typeindex::type_id<Out_>().type_info();
                }
        };

        struct GraphPathDecl : PathDecl{
                GraphPathDecl(Graph* G_, GNode* N_, GraphColouring<std::shared_ptr<TransformBase> >* T_):G{G_}, N{N_}, T{T_}{}
                virtual std::shared_ptr<PathDecl> Next(std::shared_ptr<TransformBase> ptr)override{
                        auto next = G->Node("foo");
                        auto e = G->Edge(N, next);
                        (*T)[e] = ptr;
                        return std::make_shared<GraphPathDecl>(G, next, T);
                }
        private:
                Graph* G;
                GNode* N;
                GraphColouring<std::shared_ptr<TransformBase> >* T;
        };

        struct Control : TransformControl{
                virtual void Emit(AnyType const& val)override{
                        E.push_back(val);
                }
                virtual AnyType& Arg(size_t idx){
                        assert( idx == 0 );
                        return A;
                }
                virtual void Error(std::string const& msg){
                        errors_.push_back(msg);
                }
                virtual size_t Depth()const{ return depth_; }

                virtual std::shared_ptr<PathDecl> DeclPath(){
                        if( M == nullptr )
                                M = G->Node("aux");
                        return std::make_shared<GraphPathDecl>(G, M, T);
                }
                virtual void Pass()override{
                        E.push_back(A);
                }
                virtual void Return(AnyType const& value){
                        return_ = value;
                }
                

                Graph* G;
                GNode* N;
                GraphColouring<std::shared_ptr<TransformBase> >* T;
                // arguments into the transform
                AnyType A;

                GNode* M{nullptr};


                // emitted "return" data
                std::vector<AnyType> E;
                // errors
                std::vector<std::string> errors_;

                size_t depth_{0};

                boost::optional<AnyType> return_;
        };

        struct TransformContext{
                TransformContext(){
                        head_ = G.Node("start");
                }
                std::shared_ptr<PathDecl> Start(){
                        return std::make_shared<GraphPathDecl>(&G, head_, &T);
                }


                struct StackItem{
                        StackItem(GNode* node_, AnyType A_, size_t depth_)
                                :node(node_),
                                A(A_),
                                depth(depth_)
                        {}
                        friend std::ostream& operator<<(std::ostream& ostr,                 StackItem const& self){
                                ostr << "node = " << self.node;
                                //ostr << ", A = " << self.A;
                                ostr << ", depth = " << self.depth;
                                return ostr;
                        }
                        GNode* node;
                        AnyType A;
                        size_t depth{0};
                        bool operator<(StackItem const& that)const{
                                if( depth != that.depth )
                                       return depth < that.depth;
                                return this < &that;
                        }
                };

                enum Flags{
                        F_AggregateReturn = 1,
                        F_ReturnTerminals = 2,
                };
                size_t flags_ = F_AggregateReturn | F_ReturnTerminals;

                template<class Out, class In>
                std::vector<Out> Execute(In const& val){
                        GraphColouring<std::vector<AnyType> > D;
                        D[head_].push_back(val);

                        std::priority_queue<StackItem> q;
                        q.push(StackItem{head_, val, 0});
                                
                        enum{ MaxQueueSize = 1000 };

                        std::vector<Out> result;

                        std::cout << "head_->OutEdges().size() => " << head_->OutEdges().size() << "\n"; // __CandyPrint__(cxx-print-scalar,head_->OutEdges().size())

                        for(;q.size();){
                                auto s = q.top();
                                q.pop();

                                std::cout << "q.size() => " << q.size() << "\n"; // __CandyPrint__(cxx-print-scalar,q.size())
                                std::cout << "s.node->OutEdges().size() => " << s.node->OutEdges().size() << "\n"; // __CandyPrint__(cxx-print-scalar,s.node->OutEdges().size())
                                std::cout << "s => " << s << "\n"; // __CandyPrint__(cxx-print-scalar,s)

                                if( q.size() > MaxQueueSize )
                                        throw std::domain_error("stack too large " + boost::lexical_cast<std::string>(q.size()));


                                if( s.node->OutEdges().empty() ){
                                        if( flags_ & F_AggregateReturn ){
                                                result.push_back(te::any_cast<Out>(s.A));
                                        }
                                        continue;
                                }

                                for( auto const& e : s.node->OutEdges() ){
                                        auto t = T.Color(e);

                                        Control ctrl;
                                        ctrl.G = &G;
                                        ctrl.N = e->To();
                                        ctrl.T = & T;
                                        ctrl.A = s.A;
                                        ctrl.depth_ = s.depth;


                                        std::cout << "t->Name() => " << t->Name() << "\n"; // __CandyPrint__(cxx-print-scalar,t->Name())
                                        t->ApplyImpl(&ctrl);

                                        if( ctrl.return_ ){
                                                if( flags_ & F_AggregateReturn ){
                                                        result.push_back(te::any_cast<Out>(ctrl.return_.get()));
                                                        continue;
                                                } else {
                                                        return std::vector<Out>{te::any_cast<Out>(ctrl.return_.get())};
                                                }
                                        }

                                        GNode* n  = ( ctrl.M ? ctrl.M : e->To() );
                                        for( auto const& _ : ctrl.E ){
                                                q.push(StackItem{n, _, s.depth +1 });
                                        }

                                }
                        }

                        return result;
                }
        private:
                Graph G;
                GNode* head_;
                GraphColouring<std::shared_ptr<TransformBase> > T;
                size_t counter_{0};
        };



} // CandyTransform

#endif // CANDY_TRANSFORM_H
