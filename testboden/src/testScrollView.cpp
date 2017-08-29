#include <bdn/init.h>
#include <bdn/ScrollView.h>

#include <bdn/test.h>
#include <bdn/test/testView.h>
#include <bdn/test/MockScrollViewCore.h>
#include <bdn/test/MockButtonCore.h>

#include <bdn/Button.h>

using namespace bdn;


void testSizingWithContentView(P< bdn::test::ViewWithTestExtensions<ScrollView> > pScrollView, P<IBase> pKeepAliveInContinuations, P<bdn::test::MockUiProvider> pUiProvider, std::function<Size()> getSizeFunc)
{	
	// we add a button as a content view
	P<Button> pButton = newObj<Button>();
	pButton->label() = "HelloWorld";

	Margin buttonMargin;

	SECTION("noMargin")
	{
		// do nothing
	}

	SECTION("semMargin")
	{
		pButton->margin() = UiMargin( UiLength::sem(1), UiLength::sem(2), UiLength::sem(3), UiLength::sem(4) );

		// 1 sem = 20 DIPs in our mock ui
		buttonMargin = Margin(20, 40, 60, 80 );
	}

	SECTION("dipMargin")
	{
		pButton->margin() = UiMargin(1, 2, 3, 4);

		buttonMargin = Margin(1, 2, 3, 4 );
	}

	pScrollView->setContentView( pButton );

	P<bdn::test::MockButtonCore> pButtonCore = cast<bdn::test::MockButtonCore>( pButton->getViewCore() );

	// Sanity check. Verify the fake button size. 9.75 , 19.60 per character, rounded up to 1/3 pixel size, plus 10x8 for border
	Size buttonSize( std::ceil(10*9.75*3)/3 + 10, 19 + 2.0/3 + 8);
	REQUIRE( pButtonCore->calcPreferredSize() == buttonSize );
    
	Size expectedSize = buttonSize + buttonMargin;

	// the sizing info will update asynchronously. So we need to do the
	// check async as well.
	CONTINUE_SECTION_WHEN_IDLE(getSizeFunc, expectedSize, buttonSize, buttonMargin, pKeepAliveInContinuations)
	{
		Size size = getSizeFunc();

		REQUIRE( size == expectedSize );
	};
}

enum class TestDir_
{
    horz,
    vert
};

static double& comp(Size& s, TestDir_ dir)
{
    if(dir==TestDir_::horz)
        return s.width;
    else
        return s.height;
}

static double& comp(Point& p, TestDir_ dir)
{
    if(dir==TestDir_::horz)
        return p.x;
    else
        return p.y;
}

static Point compToPoint(double c, TestDir_ dir)
{
    if(dir==TestDir_::horz)
        return Point(c, 0);
    else
        return Point(0, c);
}

static Size compToSize(double s, TestDir_ dir)
{
    if(dir==TestDir_::horz)
        return Size(s, 0);
    else
        return Size(0, s);
}

static void testScrollClientRectToVisible_Dir(
    TestDir_ dir,
    P<IBase> pKeepAliveDuringTest,
    P<ScrollView> pScrollView,    
    double initialPos,
    Size initialPosAdd,
    double targetPos,
    Size targetPosAdd,
    double targetSize,
    Size targetSizeAdd,
    double expectedPos,
    Size expectedPosAdd )
{
    Size scrolledAreaSize = pScrollView->getContentView()->size();

    initialPos += comp(initialPosAdd, dir);
    targetPos += comp(targetPosAdd, dir);
    targetSize += comp(targetSizeAdd, dir);
    expectedPos += comp(expectedPosAdd, dir);

    pScrollView->scrollClientRectToVisible( Rect( compToPoint(initialPos, dir), pScrollView->visibleClientRect().get().getSize() ) );

    BDN_CONTINUE_SECTION_WHEN_IDLE( pKeepAliveDuringTest, pScrollView, initialPos, targetPos, targetSize, expectedPos, dir)
    {
        Rect visibleRectBefore = pScrollView->visibleClientRect();

        // check if the initial position is as expected
        REQUIRE( visibleRectBefore.getPosition() == compToPoint( initialPos, dir) );

        
            
        pScrollView->scrollClientRectToVisible( Rect( compToPoint(targetPos, dir), compToSize(targetSize, dir) ) );

        BDN_CONTINUE_SECTION_WHEN_IDLE( pKeepAliveDuringTest, pScrollView, initialPos, targetPos, targetSize, expectedPos, dir, visibleRectBefore)
        {
            Rect visibleRect = pScrollView->visibleClientRect();

            REQUIRE( visibleRect.getPosition() == compToPoint( expectedPos, dir) );

            // Size should not have changed
            REQUIRE( visibleRect.getSize() == visibleRectBefore.getSize() );
        };
    };
}

static void testScrollClientRectToVisible(
    P<IBase> pKeepAliveDuringTest,
    P<ScrollView> pScrollView,
    double initialPos,
    Size initialPosAdd,
    double targetPos,
    Size targetPosAdd,
    double targetSize,
    Size targetSizeAdd,
    double expectedPos,
    Size expectedPosAdd )
{
    SECTION("vertical")
    {
        testScrollClientRectToVisible_Dir(
            TestDir_::vert,
            pKeepAliveDuringTest,
            pScrollView,
            initialPos,
            initialPosAdd,
            targetPos,
            targetPosAdd,
            targetSize,
            targetSizeAdd,
            expectedPos,
            expectedPosAdd );
    }

    SECTION("horizontal")
    {
        testScrollClientRectToVisible_Dir(
            TestDir_::horz,
            pKeepAliveDuringTest,
            pScrollView,
            initialPos,
            initialPosAdd,
            targetPos,
            targetPosAdd,
            targetSize,
            targetSizeAdd,
            expectedPos,
            expectedPosAdd );
    }
}


TEST_CASE("ScrollView", "[ui]")
{   
    SECTION("View-base")
        bdn::test::testView<ScrollView>();

	SECTION("ScrollView-specific")
	{
		P<bdn::test::ViewTestPreparer<ScrollView> > pPreparer = newObj<bdn::test::ViewTestPreparer<ScrollView> >();

		P< bdn::test::ViewWithTestExtensions<ScrollView> > pScrollView = pPreparer->createView();

		P<bdn::test::MockScrollViewCore> pCore = cast<bdn::test::MockScrollViewCore>( pScrollView->getViewCore() );
		REQUIRE( pCore!=nullptr );

		// continue testing after the async init has finished
        CONTINUE_SECTION_WHEN_IDLE(pPreparer, pScrollView, pCore)
        {
            // testView already tests the initialization of properties defined in View.
            // So we only have to test the Window-specific things here.
	        SECTION("constructWindowSpecific")
	        {
	        }

	        SECTION("changeWindowProperty")
	        {
                SECTION("contentView")
		        {
                    SECTION("!=null")
		            {
                        P<Button> pButton = newObj<Button>();
                        bdn::test::_testViewOp( 
				            pScrollView,
                            pPreparer,
				            [pScrollView, pButton, pPreparer]()
				            {
					            pScrollView->setContentView(pButton);
				            },
				            [pScrollView, pButton, pPreparer]
				            {
                                REQUIRE( pScrollView->getContentView() == cast<View>(pButton) );
				            },
                            bdn::test::ExpectedSideEffect_::invalidateSizingInfo // should have caused sizing info to be invalidated
                            | bdn::test::ExpectedSideEffect_::invalidateParentLayout // should cause a parent layout update since sizing info was invalidated
                            | bdn::test::ExpectedSideEffect_::invalidateLayout  // should have caused a layout invalidation
				        );		        
		            }


                    SECTION("null (was already null)")
		            {
                        // check if the intended precondition for the test is actually true
                        REQUIRE( pScrollView->getContentView()==nullptr );

                        // basically we only test here that there is no crash when the content view is set to null
                        // and that it does not result in a sizing info update.
                        bdn::test::_testViewOp( 
				            pScrollView,
                            pPreparer,
				            [pScrollView]()
				            {
					            pScrollView->setContentView(nullptr);
				            },
				            [pScrollView]
				            {
                                REQUIRE( pScrollView->getContentView() == nullptr);
				            },
				            0 	// should not have caused a sizing info update (since there was no change)
                                // should not have caused parent layout update
				        );		        
		            }


                    SECTION("null (was not null)")
		            {
                        P<Button> pButton = newObj<Button>();
                        pScrollView->setContentView( pButton );

                        // basically we only test here that there is no crash when the content view is set to null
                        // and that it does not result in a sizing info update.
                        bdn::test::_testViewOp( 
				            pScrollView,
                            pPreparer,
				            [pScrollView]()
				            {
					            pScrollView->setContentView(nullptr);
				            },
				            [pScrollView]()
				            {
                                REQUIRE( pScrollView->getContentView() == nullptr);
				            },
                            bdn::test::ExpectedSideEffect_::invalidateSizingInfo // should have caused sizing info to be invalidated
                            | bdn::test::ExpectedSideEffect_::invalidateParentLayout // should cause a parent layout update since sizing info was invalidated				            
                            | bdn::test::ExpectedSideEffect_::invalidateLayout  // should have caused a layout invalidation
				        );		        
		            }
		        }
	        }

            SECTION("contentViewParent")
	        {
                P<Button> pChild = newObj<Button>();

	            SECTION("parent is set directly after add")
                {
                    pScrollView->setContentView(pChild);

                    BDN_REQUIRE( pChild->getParentView() == cast<View>(pScrollView) );
                }

                SECTION("null after destroy")
                {
                    {
                        bdn::test::ViewTestPreparer<ScrollView> preparer2;

                        P< bdn::test::ViewWithTestExtensions<ScrollView> > pScrollView2 = preparer2.createView();

                        pScrollView2->setContentView(pChild);
                    }

            
                    // preparer2 is now gone, so the view is not referenced there anymore.
                    // But there may still be a scheduled sizing info update pending that holds a
                    // reference to the window.
                    // Since we want the view to be destroyed, we do the remaining test asynchronously
                    // after all pending operations are done.

                    CONTINUE_SECTION_WHEN_IDLE_WITH(
                        [pChild]()
                        {                
                            BDN_REQUIRE( pChild->getParentView() == nullptr);	    
                        });
                }
	        }
    
	        SECTION("sizing")
	        {
		        SECTION("no contentView")
		        {
			        SECTION("calcPreferredSize")
				        REQUIRE( pScrollView->calcPreferredSize()==Size(0,0) );

		        }

		        SECTION("with contentView")
		        {
			        SECTION("calcPreferredSize")
				        testSizingWithContentView(
                            pScrollView,
                            pPreparer,
                            pPreparer->getUiProvider(),
                            [pScrollView]()
                            {
                                return pScrollView->calcPreferredSize();
                            } );
		        }
	        }
            
            SECTION("contentView aligned on full pixels")
            {
                P<Button> pChild = newObj<Button>();
                pChild->label() = "hello";

                SECTION("weird child margin")
                    pChild->margin() = UiMargin( 0.12345678 );

                SECTION("weird window padding")
                    pScrollView->padding() = UiMargin( 0.12345678 );

                pScrollView->setContentView(pChild);

                CONTINUE_SECTION_WHEN_IDLE(pChild, pScrollView)
                {
                    // the mock views we use have 3 pixels per dip
                    double pixelsPerDip = 3;

                    Point pos = pChild->position();               
                
                    REQUIRE_ALMOST_EQUAL( pos.x*pixelsPerDip, std::round(pos.x*pixelsPerDip), 0.000001 );
                    REQUIRE_ALMOST_EQUAL( pos.y*pixelsPerDip, std::round(pos.y*pixelsPerDip), 0.000001 );

                    Size size = pChild->size();               
                    REQUIRE_ALMOST_EQUAL( size.width*pixelsPerDip, std::round(size.width*pixelsPerDip), 0.000001 );
                    REQUIRE_ALMOST_EQUAL( size.height*pixelsPerDip, std::round(size.height*pixelsPerDip), 0.000001 );
                };
            }

            
            SECTION("getChildList")
            {
                SECTION("empty")
                {
                    std::list< P<View> > childList;
                    pScrollView->getChildViews(childList);

                    REQUIRE( childList.empty() );
                }

                SECTION("non-empty")
                {
                    P<Button> pChild = newObj<Button>();
                    pScrollView->setContentView(pChild);

                    std::list< P<View> > childList;
                    pScrollView->getChildViews(childList);

                    REQUIRE( childList.size() == 1);
                    REQUIRE( childList.front() == cast<View>(pChild) );
                }
            }
            
            SECTION("removeAllChildViews")
            {
                SECTION("no content view")
                {
                    pScrollView->removeAllChildViews();

                    std::list< P<View> > childList;
                    pScrollView->getChildViews(childList);

                    REQUIRE( childList.empty() );
                }

                SECTION("with content view")
                {
                    P<Button> pChild = newObj<Button>();
                    pScrollView->setContentView(pChild);

                    pScrollView->removeAllChildViews();

                    REQUIRE( pScrollView->getContentView()==nullptr );
                    REQUIRE( pChild->getParentView() == nullptr );

                    std::list< P<View> > childList;
                    pScrollView->getChildViews(childList);

                    REQUIRE( childList.empty() );
                }
            }
            
            SECTION("content view detached before destruction begins")
            {            
                P<Button> pChild = newObj<Button>();
                pScrollView->setContentView( pChild );

                struct LocalTestData_ : public Base
                {
                    bool destructorRun = false;
                    int childParentStillSet = -1;
                    int childStillChild = -1;
                };

                P<LocalTestData_> pData = newObj<LocalTestData_>();

            
                pScrollView->setDestructFunc(
                    [pData, pChild]( bdn::test::ViewWithTestExtensions<ScrollView>* pWin )
                    {
                        pData->destructorRun = true;
                        pData->childParentStillSet = (pChild->getParentView()!=nullptr) ? 1 : 0;
                        pData->childStillChild = (pWin->getContentView()!=nullptr) ? 1 : 0;
                    } );

                BDN_CONTINUE_SECTION_WHEN_IDLE(pData, pChild)
                {
                    // All test objects should have been destroyed by now.
                    // First verify that the destructor was even called.
                    REQUIRE( pData->destructorRun );

                    // now verify what we actually want to test: that the
                    // content view's parent was set to null before the destructor
                    // of the parent was called.
                    REQUIRE( pData->childParentStillSet == 0 );

                    // the child should also not be a child of the parent
                    // from the parent's perspective anymore.
                    REQUIRE( pData->childStillChild == 0 );
                };
            }


            SECTION("scrollAreaToVisible and scrollPosition")
            {
                P<Button> pButton = newObj<Button>();

                // make the button bigger than the scroll view so that
                // it will scroll
                pScrollView->preferredSizeMinimum() = Size(500, 500);
                pScrollView->preferredSizeMaximum() = Size(500, 500);                
                pButton->preferredSizeMinimum() = Size(1000, 1000);
                pButton->preferredSizeMaximum() = Size(1000, 1000);

                pScrollView->horizontalScrollingEnabled() = true;
                pScrollView->setContentView(pButton);

                pPreparer->getWindow()->requestAutoSize();
                
                CONTINUE_SECTION_WHEN_IDLE(pScrollView, pButton, pPreparer)
                {
                    Size scrollViewSize = pScrollView->size();
                    Size viewPortSize = pScrollView->visibleClientRect().get().getSize();
                    Size clientSize = pButton->size();

                    // verify that the scroll view initialization was successful
                    REQUIRE( viewPortSize > Size(400, 400) );
                    REQUIRE( viewPortSize < Size(600, 600) );

                    REQUIRE( clientSize > Size(900, 900) );
                    REQUIRE( clientSize < Size(1100, 1100) );

                    SECTION("start to end")
                    {
                        SECTION("zero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, Size(),
                                0, clientSize,
                                0, Size(),
                                0, clientSize-viewPortSize);
                        }

                        SECTION("nonzero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, Size(),
                                -5, clientSize,
                                5, Size(),
                                0, clientSize-viewPortSize);
                        }
                    }

                    SECTION("end to start")
                    {
                        SECTION("zero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, clientSize-viewPortSize,
                                0, Size(),
                                0, Size(),
                                0, Size() );
                        }

                        SECTION("nonzero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, clientSize-viewPortSize,
                                0, Size(),
                                5, Size(),
                                0, Size() );
                        }
                    }

                    SECTION("start to almost end")
                    {
                        SECTION("zero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, Size(),
                                -5, clientSize,
                                0, Size(),
                                -5, clientSize-viewPortSize );
                        }

                        SECTION("nonzero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, Size(),
                                -10, clientSize,
                                5, Size(),
                                -5, clientSize-viewPortSize );
                        }
                    }

                    SECTION("end to almost start")
                    {
                        SECTION("zero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, clientSize-viewPortSize,
                                5, Size(),
                                0, Size(),
                                5, Size() );
                        }

                        SECTION("nonzero target area size")
                        {
                            testScrollClientRectToVisible(
                                pPreparer,
                                pScrollView,
                                0, clientSize-viewPortSize,
                                5, Size(),
                                5, Size(),
                                5, Size() );
                        }
                    }

                    SECTION("area already visible")
                    {
                        SECTION("start of viewport")
                        {
                            SECTION("zero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10, Size(),
                                    0, Size(),
                                    10, Size() );
                            }

                            SECTION("nonzero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10, Size(),
                                    5, Size(),
                                    10, Size() );
                            }
                        }

                        SECTION("end of viewport")
                        {
                            SECTION("zero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10, viewPortSize,
                                    0, Size(),
                                    10, Size() );
                            }

                            SECTION("nonzero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    5, viewPortSize,
                                    5, Size(),
                                    10, Size() );
                            }
                        }
                    }

                    SECTION("part of target area barely not visible")
                    {
                        SECTION("start of viewport")
                        {
                            SECTION("zero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10-1, Size(),
                                    0, Size(),
                                    10-1, Size() );
                            }

                            SECTION("nonzero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10-1, Size(),
                                    5, Size(),
                                    10-1, Size() );
                            }
                        }

                        SECTION("end of viewport")
                        {
                            SECTION("zero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10+1, viewPortSize,
                                    0, Size(),
                                    10+1, Size() );
                            }

                            SECTION("nonzero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    10+1-5, viewPortSize,
                                    5, Size(),
                                    10+1, Size() );
                            }
                        }
                    }


                    SECTION("target position outside client area")
                    {
                        SECTION("negative position")
                        {
                            SECTION("target area ends before 0")
                            {
                                SECTION("zero target area size")
                                {
                                    testScrollClientRectToVisible(
                                        pPreparer,
                                        pScrollView,
                                        10, Size(),
                                        -100, Size(),
                                        0, Size(),
                                        0, Size() );
                                }

                                SECTION("nonzero target area size")
                                {
                                    testScrollClientRectToVisible(
                                        pPreparer,
                                        pScrollView,
                                        10, Size(),
                                        -100, Size(),
                                        5, Size(),
                                        0, Size() );
                                }
                            }

                            SECTION("target area crosses 0")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    -100, Size(),
                                    150, Size(),
                                    0, Size() );
                            }

                            SECTION("target area crosses and exceeds viewport")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    -100, Size(),
                                    150, viewPortSize,
                                    
                                    // the target rect is bigger than the viewport, so we cannot
                                    // make all of it visible.
                                    // The left/top side of the target rect should get preference.
                                    0, Size() );
                            }
                        }

                        SECTION("exceeds end")
                        {
                            SECTION("target area starts after client area")
                            {
                                SECTION("zero target area size")
                                {
                                    testScrollClientRectToVisible(
                                        pPreparer,
                                        pScrollView,
                                        10, Size(),
                                        1, clientSize,
                                        0, Size(),
                                        0, clientSize-viewPortSize );
                                }

                                SECTION("nonzero target area size")
                                {
                                    testScrollClientRectToVisible(
                                        pPreparer,
                                        pScrollView,
                                        10, Size(),
                                        1, clientSize,
                                        5, Size(),
                                        0, clientSize-viewPortSize );
                                }
                            }

                            SECTION("target area crosses end")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    -10, clientSize,
                                    20, Size(),
                                    0, clientSize-viewPortSize );
                            }

                            SECTION("target area crosses end and exceeds viewport")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    -10, clientSize,
                                    20, viewPortSize,
                                    0, clientSize-viewPortSize );
                            }
                        }

                        SECTION("positive infinity")
                        {
                            SECTION("zero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    std::numeric_limits<double>::infinity(), Size(),
                                    0, Size(),
                                    0, clientSize-viewPortSize );
                            }

                            SECTION("nonzero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    std::numeric_limits<double>::infinity(), Size(),
                                    5, Size(),
                                    0, clientSize-viewPortSize );
                            }
                        }

                        SECTION("negative infinity")
                        {
                            SECTION("zero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    - std::numeric_limits<double>::infinity(), Size(),
                                    0, Size(),
                                    0, Size() );
                            }

                            SECTION("nonzero target area size")
                            {
                                testScrollClientRectToVisible(
                                    pPreparer,
                                    pScrollView,
                                    10, Size(),
                                    - std::numeric_limits<double>::infinity(), Size(),
                                    5, Size(),
                                    0, Size() );
                            }
                        }

                    }                    
                };        
            }
        };
    }
}



