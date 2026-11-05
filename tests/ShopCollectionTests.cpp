#include "ShopCollection.h"
#include <iostream>

int shopCollectionTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL shop collection: " << label << '\n';
            ++failures;
        }
    };
    const auto resource = nativesaves::makeItem(11, 9);
    const auto weapon = nativesaves::makeItem(3, 1);
    gameshop::Collection shop;
    gameshop::BuyBack history{{resource, 0}, {weapon, -7}, {resource, 2}, {resource, 2}};
    int generated = 0;
    const auto stock = [&] {
        ++generated;
        return std::vector<gameshop::Offer>{{resource, 2}, {resource, 3}, {resource, -1},       {weapon, 1},
                                            {weapon, 1},   {resource, 0}, {gameshop::Item{}, 9}};
    };
    shop.rows.resize(5);
    shop.rows[4] = {{99, weapon}};
    shop.rebuild(0, history, stock);
    check(generated == 1 && shop.rows.size() == 2 && shop.rows[1].empty() && shop.rows[0].size() == 3 &&
              shop.rows[0][0].count == -1 && shop.rows[0][1].count == 1 && shop.rows[0][2].count == 1,
          "stock uses shared insertion, merges stackables and preserves independent nonstackable rows");
    const auto retainedResource = shop.rows[0][0].item;
    shop.rebuild(1, history, stock);
    check(generated == 1 && shop.rows[0].empty() && shop.rows[1].size() == 4 && shop.rows[1][0].count == 2 &&
              shop.rows[1][1].count == 2 && shop.rows[1][2].count == -7 && shop.rows[1][3].count == 0,
          "buyback skips generation, clears inactive stock and copies newest-first without quantity/dedup filters");
    history.back().first = weapon;
    history.back().second = 37;
    check(shop.rows[1][0].item == retainedResource && shop.rows[1][0].count == 2,
          "retained buyback rows own values independently of history mutation");
    shop.rebuild(0, history, [] { return std::vector<gameshop::Offer>{}; });
    check(shop.rows.size() == 2 && shop.rows[0].empty() && shop.rows[1].empty() && history.size() == 4,
          "missing merchant/empty stock clears copied rows without consuming retained history");
    history.clear();
    shop.rebuild(1, history, stock);
    check(generated == 1 && shop.rows[0].empty() && shop.rows[1].empty(),
          "empty buyback source does not preserve prior rows or invoke the merchant generator");
    shop.rebuild(4, history, stock);
    check(generated == 2 && !shop.rows[0].empty() && shop.rows[1].empty(),
          "every tab value except exactly one selects stock without normalizing the caller's tab");
    std::cout << "Shop retained collections: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
