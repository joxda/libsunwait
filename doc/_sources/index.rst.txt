.. libsunwait documentation master file

Documentation for libsunwait
==========================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:


This is a port to a C++ library of the ``sunwait`` executable (licencsed under
the GPLv3) written by Dan Risacher based on codes by Paul Schlyter and
with contributions of others mentioned in the original code 
( https://github.com/risacher/sunwait )
It provide a class with functions to wait for sunrise and set, poll whether
it is day or night, list sunrise and set times. Instead of sunrise or set
corresponding twilight times can be considered, as well as times offset 
by a fixed amount from any of these.

API of libsunwait
-----------------------------------------



SunWait 
^^^^^^^
.. doxygenclass:: SunWait
   :project: libsunwait
   :members:



Preprocessor defines
^^^^^^^^^^^^^^^^^^^^
.. doxygengroup:: TwilightAngles
   :project: libsunwait
   :members:

.. doxygengroup:: ReturnCodes
   :project: libsunwait
   :members:
 
.. doxygengroup:: PolarDayNight
   :project: libsunwait
   :members:




Index
==================

* :ref:`genindex`


