Transit Planner - an experiment on transfer patterns robustness
===============================================================


Description
-----------
Transit Planner is an *efficient route planner* for public transportation networks.
Its implementation is based on the transfer patterns precomputation, providing
ultra-fast query times on large networks.

The planner was used to conduct experiments on the *reliablity of transfer
patterns* in the presence of *real-time updates* of the network, like delays or
connectivity problems.


Authors
-------
Transit Planner and all its auxiliary modules were developed by [Eugen Sawin](mailto:esawin@gmail.com), [Philip Stahl](mailto:stahl@informatik.uni-freiburg.de) and [Jonas
Sternisko](jonas.sternisko@mars.uni-freiburg.de) during a master team project
at the *University of Freiburg in Germany*.


Licensing
---------
Transit Planner and all its auxiliary modules are released under the *GNU General Public License Version 3*, see the COPYING notice for the full license text.


Latest Version
--------------
To obtain the latest version of Transit Planner, visit the code section of the
[project overview website](http://stromboli.informatik.uni-freiburg.de/student-projects/philip+jonas+eugen).


Requirements
------------
  * POSIX.1b-compliant operating system (librt)
  * GNU GCC 4.6
  * GNU Make
  * Python 2.7 (only for style checking and the documentation server)


Dependencies
------------
  * Boost C++ Libraries 1.46
  * OpenMP 4.6
  * Google C++ Test Framework 1.6 (only required for testing)
  * Google C++ Style Guide Linter (included, only required for style checking)


Building
---------------
  To build Transit Planner use:  
  `$ make compile`

  For performance measuring use the more optimised version:  
  `$ make opt`

  Alternatively you can build, test and check style at once using:  
  `$ make`


Testing (depends on gtest)
------------------------------
  To build and run the unit tests use:  
  `$ make test`


Checking Style
--------------
  To test code style conformance with the [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml) use:  
  `$ make checkstyle`


Usage
-----
  To start the Transit Planner server use:  
  `$ ./build/ServerMain [-i <datasets>] [-m <num-workers>] [-p <port>]`

  where `<dataset>` are the GTFS directories of the transit networks,  
  `<num-workers>` are the maximum number of threads to be used and  
  `<port>` is the port to be used for listening.  

  To show the full usage help use:  
  `$ ./build/ServerMain -h`


Show the documentation
----------------------
  In the www-directory, run
  '$ python -m SimpleHTTPServer PORT'
  and find the documentation at http://localhost:PORT or serve the 
  contents with any other server.

