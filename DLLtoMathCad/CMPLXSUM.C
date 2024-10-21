#include "mcadincl.h"
    
    
LRESULT cmplxsumFunction(   LPCOMPLEXSCALAR     c,
                                LPCCOMPLEXSCALAR    a,
                                LPCCOMPLEXSCALAR    b   );

    FUNCTIONINFO    cmplxsum = 
    { 
    "cmplxsum",                         // Name by which mathcad will recognize the function
    "a,b",                              // cmplxsum will be called as cmplxsum(a,b)
    "complex sum of scalars a and b",       // description of cmplxsum(a,b)
    (LPCFUNCTION)cmplxsumFunction,      // pointer to the executible code
    COMPLEX_SCALAR, 
    2,                  // the return type is also a complex scalar
    { COMPLEX_SCALAR, COMPLEX_SCALAR}   // both arguments are complex scalars
    };
    

    
    // this code executes the addition
    LRESULT cmplxsumFunction(   LPCOMPLEXSCALAR     c,  //put return value here
                                LPCCOMPLEXSCALAR    a,  // 1st argument
                                LPCCOMPLEXSCALAR    b   ) // 2nd argument
    {
        
        //  add real parts of a and b
        //  and put them into c
        c->real = a->real + b->real;
        c->imag = a->imag + b->imag;
        
        
        return 0;               // return 0 to indicate there was no error
            
    }           
    
BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved);

BOOL WINAPI DllEntryPoint (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:         // DLL is attaching to the address space of the current process.

      if (!_CRT_INIT(hDLL, dwReason, lpReserved)) 
        return FALSE;
    
        CreateUserFunction( hDLL, &cmplxsum ); 
      break;

    case DLL_THREAD_ATTACH:         // A new thread is being created in the current process.
    case DLL_THREAD_DETACH:        // A thread is exiting cleanly.
    case DLL_PROCESS_DETACH:      // The calling process is detaching the DLL from its address space.

      if (!_CRT_INIT(hDLL, dwReason, lpReserved)) 
        return FALSE;
      break;

  }
  return TRUE;
}

