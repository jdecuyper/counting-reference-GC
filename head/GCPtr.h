#include <iostream>
#include <list>
#include <typeinfo>
#include <cstdlib>
#include "GCInfo.h"
#include "Iter.h"

#define DISPLAY

using namespace std;

template <class T, int size=0> class GCPtr {
    // gclist maintains the garbage collection list.
    static list< GCInfo<T> > gclist;
    
    // addr points to the allocated memory to which
    // this GCPtr pointer currently points.
    T *addr;
    
    /* isArray is true if this GCPtr points
    to an allocated array. It is false
    otherwise. */
    bool isArray; // true if pointing to array
    
    // If this GCPtr is pointing to an allocated
    // array, then arraySize contains its size.
    unsigned arraySize; // size of the array
    
    static bool first; // true when first GCPtr is created
    
    // Return an iterator to pointer info in gclist.
    typename list<GCInfo<T> >::iterator findPtrInfo(T *ptr);
    
    public:
        // Define an iterator type for GCPtr<T>.
        typedef Iter<T> GCiterator;
        
        // Construct both initialized and uninitialized objects.
        GCPtr(T *t=NULL) {
        
        // Register shutdown() as an exit function.
        if(first) atexit(shutdown);
        first = false;
        typename list<GCInfo<T> >::iterator p;
        p = findPtrInfo(t);
        
        // If t is already in gclist, then
        // increment its reference count.
        // Otherwise, add it to the list.
        if(p != gclist.end())
          p->refcount++; // increment ref count
        else {
          // Create and store this entry.
          GCInfo<T> gcObj(t, size);
          gclist.push_front(gcObj);
        }
        addr = t;
        arraySize = size;
        if(size > 0) isArray = true;
        else isArray = false;
        
        #ifdef DISPLAY
          cout << "Constructing GCPtr. ";
          if(isArray)
            cout << " Size is " << arraySize << endl;
          else
            cout << endl;
        #endif
        }
        
        // Copy constructor.
        GCPtr(const GCPtr &ob) {
            // typename states that the name that follows should be treated as a type. 
            // Otherwise, names are interpreted to refer to non-types.
            typename list<GCInfo<T> >::iterator p;
            p = findPtrInfo(ob.addr);
            p->refcount++; // increment ref count
            addr = ob.addr;
            arraySize = ob.arraySize;
            if(arraySize > 0) isArray = true;
            else isArray = false;
            #ifdef DISPLAY
            cout << "Constructing copy.";
            if(isArray)
            cout << " Size is " << arraySize << endl;
            else
            cout << endl;
            #endif
        }

        // Destructor for GCPTr.
        ~GCPtr();
        
        // Collect garbage. Returns true if at least
        // one object was freed.
        static bool collect();
        
        // Overload assignment of pointer to GCPtr.
        T *operator=(T *t);
        
        // Overload assignment of GCPtr to GCPtr.
        GCPtr &operator=(GCPtr &rv);
        
        // Return a reference to the object pointed
        // to by this GCPtr.
        T &operator*() {
          return *addr;
        }
        
        // Return the address being pointed to.
        T *operator->() { return addr; }
        
        // Return a reference to the object at the
        // index specified by i
        T &operator[](int i) {
          return addr[i];
        }
        
        // Conversion function to T *.
        operator T *() { return addr; }
        
        // Return an Iter to the start of the allocated memory.  
        Iter<T> begin() {
            int s;
            if(isArray) s = arraySize;
            else s = 1;
            return Iter<T>(addr, addr, addr + s);
        }
        
        // Return an Iter to one past the end of an allocated array.
        Iter<T> end() {
        int s;
        if(isArray) s = arraySize;
        else s = 1;
        return Iter<T>(addr + size, addr, addr + s);
        }
        
        // Return the size of gclist for this type
        // of GCPtr.
        static int gclistSize() { return gclist.size(); }
        
        // A utility function that displays gclist.
        static void showlist();
        
        // Clear gclist when program exits.
        static void shutdown();
};

// Creates storage for the static variables
template <class T, int size> list<GCInfo<T> > GCPtr<T, size>::gclist;
template <class T, int size>  bool GCPtr<T, size>::first = true;

// Destructor for GCPtr.
template <class T, int size>
GCPtr<T, size>::~GCPtr() {
  typename list<GCInfo<T> >::iterator p;
  p = findPtrInfo(addr);
  if(p->refcount) p->refcount--; // decrement ref count
  #ifdef DISPLAY
    cout << "GCPtr going out of scope.\n";
  #endif
  // Collect garbage when a pointer goes out of scope.   
  collect();
  // For real use, you might want to collect
  // unused memory less frequently, such as after
  // gclist has reached a certain size, after a
  // certain number of GCPtrs have gone out of scope,
  // or when memory is low.
}

// Collect garbage.  Returns true if at least
// one object was freed.
template <class T, int size>
bool GCPtr<T, size>::collect() {
  bool memfreed = false;
  #ifdef DISPLAY
    cout << "Before garbage collection for ";
    showlist();
  #endif
  typename list<GCInfo<T> >::iterator p;
  do {
    // Scan gclist looking for unreferenced pointers.
    for(p = gclist.begin(); p != gclist.end(); p++) {
      // If in-use, skip.
      if(p->refcount > 0) continue;
      memfreed = true;
      // Remove unused entry from gclist.
      gclist.remove(*p);
      // Free memory unless the GCPtr is null.
      if(p->memPtr) {
        if(p->isArray) {
          #ifdef DISPLAY
            cout << "Deleting array of size "
                 << p->arraySize << endl;
          #endif
          delete[] p->memPtr; // delete array
        }
        else {
          #ifdef DISPLAY
            cout << "Deleting: "
                 << *(T *) p->memPtr << "\n";
          #endif
          delete p->memPtr; // delete single element
        }
      }
      // Restart the search.
      break;
    }
  } while(p != gclist.end());
  #ifdef DISPLAY
    cout << "After garbage collection for ";
    showlist();
  #endif
  return memfreed;
}

// Overload assignment of pointer to GCPtr.
template <class T, int size>
T * GCPtr<T, size>::operator=(T *t) {
  typename list<GCInfo<T> >::iterator p;
  // First, decrement the reference count
  // for the memory currently being pointed to.
  p = findPtrInfo(addr);
  p->refcount--;
  // Next, if the new address is already
  // existent in the system, increment its
  // count. Otherwise, create a new entry
  // for gclist.
  p = findPtrInfo(t);
  if(p != gclist.end())
    p->refcount++;
  else {
    // Create and store this entry.
    GCInfo<T> gcObj(t, size);
    gclist.push_front(gcObj);
  }
  addr = t; // store the address.
  return t;
}

// Overload assignment of GCPtr to GCPtr.
template <class T, int size>
GCPtr<T, size> & GCPtr<T, size>::operator=(GCPtr &rv) {
  typename list<GCInfo<T> >::iterator p;
  // First, decrement the reference count
  // for the memory currently being pointed to.
  p = findPtrInfo(addr);
  p->refcount--;
  // Next, increment the reference count of
  // the new address.
  p = findPtrInfo(rv.addr);
  p->refcount++; // increment ref count
  addr = rv.addr;// store the address.
  return rv;
}
// A utility function that displays gclist.
template <class T, int size>
void GCPtr<T, size>::showlist() {
  typename list<GCInfo<T> >::iterator p;
  cout << "gclist<" << typeid(T).name() << ", "
       << size << ">:\n";
  cout << "memPtr     refcount    value\n";
  if(gclist.begin() == gclist.end()) {
    cout << "           -- Empty --\n\n";
    return;
  }
  for(p = gclist.begin(); p != gclist.end(); p++) {
    cout << "[" << (void *)p->memPtr << "]"
      << "       " << p->refcount << "       ";
    if(p->memPtr) cout << " " << *p->memPtr;
    else cout << "  ---";
    cout << endl;
  }
  cout << endl;
}
// Find a pointer in gclist.
template <class T, int size>
  typename list<GCInfo<T> >::iterator
  GCPtr<T, size>::findPtrInfo(T *ptr) {
  typename list<GCInfo<T> >::iterator p;
  // Find ptr in gclist.
  for(p = gclist.begin(); p != gclist.end(); p++)
    if(p->memPtr == ptr)
      return p;
  return p;
}
// Clear gclist when program exits.
template <class T, int size>
void GCPtr<T, size>::shutdown() {
  if(gclistSize() == 0) return; // list is empty
  typename list<GCInfo<T> >::iterator p;
  for(p = gclist.begin(); p != gclist.end(); p++) {
    // Set all reference counts to zero
    p->refcount = 0;
  }
  #ifdef DISPLAY
    cout << "Before collecting for shutdown() for "
    << typeid(T).name() << "\n";
  #endif
  collect();
  #ifdef DISPLAY
    cout << "After collecting for shutdown() for "
         << typeid(T).name() << "\n";
  #endif
}


