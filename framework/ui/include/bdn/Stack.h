#pragma once

#include <bdn/View.h>
#include <deque>

namespace bdn
{
    class Stack : public View
    {
      public:
        static constexpr char coreTypeName[] = "bdn.Stack";

        struct StackEntry
        {
            std::shared_ptr<View> view;
            String title;
        };

      public:
        Stack();
        virtual ~Stack();

        void pushView(std::shared_ptr<View> view, String title);
        void popView();

        const std::deque<StackEntry> &stack() const { return _stack; }

      public:
        String getCoreTypeName() const override { return coreTypeName; }
        virtual void _initCore() override;
        virtual void _deinitCore() override;

      private:
        std::deque<StackEntry> _stack;
    };
}
