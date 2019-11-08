LilyFuture
==========

This is a tiny header-only library adding some utility function to ease the use of [`std::future`](https://en.cppreference.com/w/cpp/thread/future).

Example
-------

This is an example of concurrent cooking easily achieved using this 

```C++
#include <iostream>
#include <LilyFuture.h>

int main(int argc, char **argv) {

	std::future<Ingredients> fIngredients = std::async(fetchIngredients, "flour", "eggs", "water", "milk", "vanilla");
	// This one is shared because we use it several times
	std::shared_future<Kitchenware> fKitchenware = std::async(findBowlAndPan).share();

	auto fBatter = then(fIngredients, fKitchenware, [](Ingredients ingredients, Kitchenware kitchenware) {
		return whisk(kitchenware.bowl, ingredients.all());
	});

	// This will likely run at the same time as the ingredients are whisked
	auto fHotPan = then(fKitchenware, [](Kitchenware kitchenware) {
		return warmUpDuring5Minutes(kitchenware.pan);
	});

	auto fCookedCrepe = then(fBatter, fHotPan, [](Batter batter, HotPan pan) {
		return pan.cook(batter);
	}).share(); // shared as well

	auto fWashedDishes = then(fKitchenware, fCookedCrepe, [](Kitchenware kitchenware, auto _ /* we don't really care about the result here */){
		return kitchenware.washAll();
	});

	auto done = then(fCookedCrepe, fWashedDishes, [](auto _1, auto _2) {});

	try { done.get(); std::cout << "Enjoy your meal!" << std::endl; return EXIT_SUCCESS; }
	catch (...) { std::err << "An error occured :(" << std::endl; return EXIT_FAILURE; }
}

```

Reference
---------

**`then()`**  
Takes some futures or shared futures and a callback to run when all values are available.  
It requires a future to the type returned by the callback.  
It may also be called "bind()" or "require()" depending on you mindset.  
Use shared futures when they occure in several calls to "then", raw futures otherwise.


**`all()`**  
Transforms a vector of futures to a future vector, ready when all futures are.
