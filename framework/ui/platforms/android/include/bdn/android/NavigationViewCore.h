#pragma once

#include <bdn/ui/NavigationView.h>

#include <bdn/android/ContainerViewCore.h>
#include <bdn/android/WindowCore.h>

namespace bdn::ui::android
{
    class NavButtonHandler;

    class NavigationViewCore : public ViewCore, public NavigationView::Core
    {
      public:
        NavigationViewCore(const std::shared_ptr<ViewCoreFactory> &viewCoreFactory);
        ~NavigationViewCore() override;

      public:
        void pushView(std::shared_ptr<View> view, std::string title) override;
        void popView() override;
        std::vector<std::shared_ptr<View>> childViews() const override;

      public:
        void visitInternalChildren(const std::function<void(std::shared_ptr<View::Core>)> &function) override;

        bool handleBackButton();
        void viewIsChanging(const std::shared_ptr<ViewCore> &from, const std::shared_ptr<ViewCore> &to);

      private:
        void updateCurrentView(bool first, bool enter);
        void reLayout();

      private:
        struct StackEntry
        {
            std::shared_ptr<ContainerView> container;
            std::shared_ptr<View> view;
            std::string title;
        };

        std::deque<StackEntry> _stack;
    };
}
