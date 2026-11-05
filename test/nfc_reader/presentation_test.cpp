#include "NfcPresentation.h"
#include <cassert>
int main(){
 NfcPresentation p;std::vector<uint8_t> a{1,2,3,4},b{5,6,7,8};
 assert(p.accept(a,0));assert(!p.accept(a,20000)); // Time alone never re-arms a held UID.
 p.absent(20999);assert(!p.accept(a,20999));p.absent(22000);assert(p.accept(a,22000));
 assert(p.accept(b,22001));assert(p.accept(a,22002));
 assert(!p.accept({1,2,3},23000));assert(!p.accept(a,23000));
 p.reset();assert(p.accept(a,0xfffffff0));p.absent(500);assert(!p.accept(a,500));
 p.absent(1500);assert(p.accept(a,1500)); // Timer wrap.
}
