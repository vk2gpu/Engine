#pragma once

#include "core/debug.h"

#include <utility>

namespace Core
{
	namespace Detail
	{
		template<typename SIGNATURE>
		class FunctionObjectBase;

		template<typename RET, typename... ARGS>
		class FunctionObjectBase<RET(ARGS...)>
		{
		public:
			using func_obj_base = Detail::FunctionObjectBase<RET(ARGS...)>;

			FunctionObjectBase() {}
			virtual ~FunctionObjectBase() {}

			virtual func_obj_base* DoCopy(void* storage) const = 0;
			virtual RET DoCall(ARGS&&... args) const = 0;
		};

		template<typename CALLABLE, typename SIGNATURE>
		class FunctionObject;

		template<typename CALLABLE, typename RET, typename... ARGS>
		class FunctionObject<CALLABLE, RET(ARGS...)> : public FunctionObjectBase<RET(ARGS...)>
		{
		public:
			using func_obj_base = Detail::FunctionObjectBase<RET(ARGS...)>;
			using func_obj = Detail::FunctionObject<CALLABLE, RET(ARGS...)>;

			FunctionObject(const CALLABLE&& callable)
			    : callable_(std::forward<const CALLABLE>(callable))
			{
			}

			virtual ~FunctionObject() {}

			func_obj_base* DoCopy(void* storage) const override
			{
				return new(storage) func_obj(std::forward<const CALLABLE>(callable_));
			}
			RET DoCall(ARGS&&... args) const override { return callable_(std::forward<ARGS>(args)...); }

			CALLABLE callable_;
		};
	}

	template<typename SIGNATURE, i32 MAX_SIZE = 32>
	class Function;

	template<typename RET, typename... ARGS, i32 MAX_SIZE>
	class Function<RET(ARGS...), MAX_SIZE>;

	template<typename SIGNATURE>
	struct IsFunction
	{
		static constexpr bool value = false;
	};

	template<typename RET, typename... ARGS, i32 MAX_SIZE>
	struct IsFunction<Function<RET(ARGS...), MAX_SIZE>>
	{
		static constexpr bool value = true;
	};

	template<typename RET, typename... ARGS, i32 MAX_SIZE>
	class Function<RET(ARGS...), MAX_SIZE>
	{
	public:
		using func_obj_base = Detail::FunctionObjectBase<RET(ARGS...)>;

		template<typename CALLABLE>
		using func_obj = Detail::FunctionObject<CALLABLE, RET(ARGS...)>;

		Function() {}
		Function(nullptr_t) {}

		Function(const Function& func)
		{
			copy(func.funcObj_);
		}

		Function& operator=(const Function& func)
		{
			copy(func.funcObj_);
			return *this;
		}

		template<typename CALLABLE, typename = typename std::enable_if_t<!IsFunction<CALLABLE>::value>>
		Function(CALLABLE callable)
		{
			static_assert(sizeof(func_obj<CALLABLE>) <= MAX_SIZE, "MAX_SIZE too small for wrapped callable.");
			funcObj_ = new(&storage_[0]) func_obj<CALLABLE>(std::move(callable));
		}

		~Function()
		{
			destroy();
		}

		RET operator()(ARGS... args) const
		{
			DBG_ASSERT(funcObj_);
			return funcObj_->DoCall(std::forward<ARGS>(args)...);
		}

		operator bool() { return !!funcObj_; }

	private:
		void destroy()
		{
			if(funcObj_)
			{
				funcObj_->~func_obj_base();
				funcObj_ = nullptr;
			}
		}

		void copy(func_obj_base* other)
		{
			if(other)
				funcObj_ = other->DoCopy(storage_);
			else
				destroy();
		}

		u8 storage_[MAX_SIZE];
		func_obj_base* funcObj_ = nullptr;
	};
} // namespace Core
