#include <iostream>

#include "GCPtr.h"

using namespace std;

int main(int argc, char *argv[])
{
    GCPtr<int> p;
    try {
       p = new int;
    } catch(bad_alloc exc) {
      cout << "Allocation failure!\n";
      return 1;
    }
    *p = 88;
    cout << "Value at p is: " << *p << endl;
    int k = *p;
    cout << "k is " << k << endl;

    cin.get();
    return 0;
}
