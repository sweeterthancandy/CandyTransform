#ifndef CANDY_TRANSFORM_H
#define CANDY_TRANSFORM_H

#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <list>

#include <boost/type_index.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/typeid_of.hpp>
#include <boost/mpl/vector.hpp>


namespace CandyTransform{

        struct TransformControl;

        namespace te = boost::type_erasure;
        namespace mpl = boost::mpl;

        using AnyType = te::any<
            mpl::vector<
                te::typeid_<>,
                te::ostreamable<>,
                te::copy_constructible<>,
                te::assignable<>
            >
        >;

        /*
         * Transforms are to be typed, so this just earses that, and does run time
         * type checking for sugar
         */
        struct TransformBase{
                virtual ~TransformBase()=default;
                virtual void ApplyImpl(TransformControl* ctrl)=0;
        protected:
                virtual boost::typeindex::type_index GetInType()const=0;
                virtual boost::typeindex::type_index GetOutType()const=0;
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
                 * Be explisit of the continuation
                 */
                virtual void Next(std::shared_ptr<TransformBase> ptr)=0;
                /*
                 * Retreive an argument
                 */
                virtual AnyType& Arg(size_t idx=0)=0;
                /*
                 * find out how deep we are in the path
                 */
                virtual size_t Depth()const=0;
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

        /*
         * This library is designed to run execution on paths. A path can either 
         * be know before it is started, ei
         *           startup() -> do_this() -> do_that() -> shutdown(),
         * or the complete path might not be known untill runtime, in that case 
         * we only have an inital path(), and the path() is decided as the path
         * is traversed. An intended application is an algebraic solver, for which
         * the path is dependant on the clasification of the problem, and many
         * paths which are unsuccessfull.
         */
        struct TransformPath{

                /* 
                 * This is used to declare the path statically
                 */
                TransformPath* Next(std::shared_ptr<TransformBase> trans){
                        auto next = std::make_shared<TransformPath>();
                        // depth_, in_ is set jit
                        next->trans_ = trans;
                        road_.push_back(next);
                        return next.get();
                }


                // we don't clone  out_ or road_
                std::shared_ptr<TransformPath> ShallowClone()const{
                        auto ptr = std::make_shared<TransformPath>();
                        ptr->depth_ = depth_;
                        ptr->in_    = in_;
                        ptr->trans_ = trans_;
                        ptr->next_  = next_;
                        return ptr;
                }

                void Display(std::ostream& ostr = std::cout)const{
                        ostr << "depth_ = " << depth_ << "\n";
                        typedef std::vector<AnyType>::const_iterator CI0;
                        const char* comma = "";
                        ostr << "in_" << " = [";
                        for(CI0 iter= in_.begin(), end=in_.end();iter!=end;++iter){
                                ostr << comma << *iter;
                                comma = ", ";
                        }
                        ostr << "]\n";
                        comma = "";
                        ostr << "out_" << " = [";
                        for(CI0 iter= out_.begin(), end=out_.end();iter!=end;++iter){
                                ostr << comma << *iter;
                                comma = ", ";
                        }
                        ostr << "]\n";
                        typedef std::vector<std::shared_ptr<TransformBase> >::const_iterator CI1;
                        comma = "";
                        ostr << "next_" << " = {";
                        for(CI1 iter= next_.begin(), end=next_.end();iter!=end;++iter){
                                ostr << comma << *iter;
                                comma = ", ";
                        }
                        ostr << "}\n";
                }
                size_t depth_{0};
                std::vector<AnyType> in_;
                std::shared_ptr<TransformBase> trans_;
                std::vector<AnyType> out_;
                // this is a vector of next steps, ie each path will be one of these
                std::vector<std::shared_ptr<TransformBase> > next_;

                // this is the sequence of transform to be applied. The idea
                // is that we might have some algorithm where a transform reques
                // something else to be applied, but at the same time we have a
                // static path. At the end of the day all static paths can be
                // expressed in the transform themselfs, but the value of having
                // self contained transforms. For example we almost always want to
                // do constant folding, ie 0 + 0, but sometimes we don't want to
                // expand ( x + 2 ) ^2, and sim sometimes we want y + y^2, and
                // sometimes y*(y*y).
                std::list<std::shared_ptr<TransformPath> > road_;

        };

        struct TransformStep : std::vector<std::shared_ptr<TransformPath> >{};

        struct Control : TransformControl{
                Control(TransformPath* path):path_{path}{}
                virtual void Emit(AnyType const& val)override{
                        path_->out_.push_back(val);
                }
                virtual void Next(std::shared_ptr<TransformBase> ptr)override{
                        path_->next_.push_back(ptr);
                }
                virtual AnyType& Arg(size_t idx){
                        return path_->in_.at(idx);
                }
                virtual void Error(std::string const& msg){
                        errors_.push_back(msg);
                }
                virtual size_t Depth()const{ return path_->depth_; }
                std::vector<std::string> errors_;
        private:
                std::vector<AnyType> args_;
                TransformPath* path_;
        };


        template<class Out>
        struct TransformContext{
                enum{ Debug = 0 };
                template<class In>
                Out ExecSingle(In const& val){
                        auto ret = Execute(val);
                        if( ret.size() != 1 ){
                                std::cout << "ret.size() => " << ret.size() << "\n"; // __CandyPrint__(cxx-print-scalar,ret.size())
                                BOOST_THROW_EXCEPTION(std::domain_error("expected single argument"));
                        }
                        return ret.back();
                }
                template<class In>
                std::vector<Out> operator()(In const& val){
                        return Execute(val);
                }
                template<class In>
                std::vector<Out> Execute(In const& val){

                        auto step = proto_; // TODO clone

                        step.front()->in_.push_back(val);
                        
                        std::vector<Out> result;

                        size_t step_idx =0 ;
                        for(;step.size();++step_idx){
                                TransformStep next_step;

                                for(auto& _ : step){
                                        Control ctrl( _.get() );

                                        _->trans_->ApplyImpl( &ctrl );
                                        if( Debug ){
                                                _->Display();
                                        }

                                        for(auto const& e : ctrl.errors_){
                                                OnError(e);
                                        }

                                        /*
                                                If nothing was emitting, we take the parameter
                                         */
                                        std::vector<AnyType>* out = ( _->out_.size() ? &_->out_ : &_->in_ );
                                        if( _->next_.size() != 0 ){
                                                for(auto& in : *out){
                                                        for(auto const& next : _->next_){
                                                                next_step.emplace_back(std::make_shared<TransformPath>());

                                                                next_step.back()->trans_ = next;
                                                                next_step.back()->in_.push_back(in);
                                                                next_step.back()->depth_ = _->depth_+1;
                                                        }
                                                }
                                        } else if( _->road_.size() != 0 ){
                                                // only going to emit one, can only have one road


                                                for(auto& in : *out){
                                                        auto head    = _->road_.front()->ShallowClone();
                                                        
                                                        BOOST_ASSERT( head->next_.size() == 0 );

                                                        head->depth_ = _->depth_+1;
                                                        head->in_.clear();
                                                        head->in_.push_back(in);
                                                        head->road_  = _->road_;
                                                        head->road_.pop_front();

                                                        next_step.push_back(head);
                                                }
                                        } else {
                                                // terminal, put them in result
                                                for(auto const& ret : *out){
                                                        result.push_back(te::any_cast<Out>(ret));
                                                }
                                        }
                                }
                                step = std::move(next_step);

                                // dispatch to callback
                                //OnStep(step);
                        }

                        return result;
                                      
                }
                TransformPath* Start(){
                        proto_.emplace_back(std::make_shared<TransformPath>());
                        struct Dummy : TransformBase{
                                virtual boost::typeindex::type_index GetInType()const{
                                        return boost::typeindex::type_id<void>().type_info();
                                }
                                virtual boost::typeindex::type_index GetOutType()const{
                                        return boost::typeindex::type_id<void>().type_info();
                                }
                                virtual void ApplyImpl(TransformControl* ctrl){}
                        };
                        proto_.back()->trans_ = std::make_shared<Dummy>();
                        return proto_.back().get();
                }
                
                virtual void OnStep(TransformStep const& step){}
                virtual void OnDebugMessage(std::string const& s){
                        std::cerr << s << "\n";
                }
                virtual void OnError(std::string const& s){
                        std::cerr << s << "\n";
                }

        private:
                std::vector<std::shared_ptr<TransformPath> > proto_;
        };


} // CandyTransform

#endif // CANDY_TRANSFORM_H
