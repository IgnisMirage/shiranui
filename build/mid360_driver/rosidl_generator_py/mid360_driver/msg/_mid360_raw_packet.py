# generated from rosidl_generator_py/resource/_idl.py.em
# with input from mid360_driver:msg/Mid360RawPacket.idl
# generated code does not contain a copyright notice

# This is being done at the module level and not on the instance level to avoid looking
# for the same variable multiple times on each instance. This variable is not supposed to
# change during runtime so it makes sense to only look for it once.
from os import getenv

ros_python_check_fields = getenv('ROS_PYTHON_CHECK_FIELDS', default='')


# Import statements for member types

# Member 'data'
import array  # noqa: E402, I100

import builtins  # noqa: E402, I100

# Member 'livox_timestamp'
import numpy  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_Mid360RawPacket(type):
    """Metaclass of message 'Mid360RawPacket'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('mid360_driver')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'mid360_driver.msg.Mid360RawPacket')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__mid360_raw_packet
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__mid360_raw_packet
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__mid360_raw_packet
            cls._TYPE_SUPPORT = module.type_support_msg__msg__mid360_raw_packet
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__mid360_raw_packet

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class Mid360RawPacket(metaclass=Metaclass_Mid360RawPacket):
    """Message class 'Mid360RawPacket'."""

    __slots__ = [
        '_data_type',
        '_dot_num',
        '_time_interval',
        '_udp_cnt',
        '_frame_cnt',
        '_time_type',
        '_livox_timestamp',
        '_data',
        '_check_fields',
    ]

    _fields_and_field_types = {
        'data_type': 'uint8',
        'dot_num': 'uint16',
        'time_interval': 'uint16',
        'udp_cnt': 'uint16',
        'frame_cnt': 'uint8',
        'time_type': 'uint8',
        'livox_timestamp': 'uint8[8]',
        'data': 'sequence<uint8>',
    }

    # This attribute is used to store an rosidl_parser.definition variable
    # related to the data type of each of the components the message.
    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint16'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint16'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint16'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint8'), 8),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.BasicType('uint8')),  # noqa: E501
    )

    def __init__(self, **kwargs):
        if 'check_fields' in kwargs:
            self._check_fields = kwargs['check_fields']
        else:
            self._check_fields = ros_python_check_fields == '1'
        if self._check_fields:
            assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
                'Invalid arguments passed to constructor: %s' % \
                ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.data_type = kwargs.get('data_type', int())
        self.dot_num = kwargs.get('dot_num', int())
        self.time_interval = kwargs.get('time_interval', int())
        self.udp_cnt = kwargs.get('udp_cnt', int())
        self.frame_cnt = kwargs.get('frame_cnt', int())
        self.time_type = kwargs.get('time_type', int())
        if 'livox_timestamp' not in kwargs:
            self.livox_timestamp = numpy.zeros(8, dtype=numpy.uint8)
        else:
            self.livox_timestamp = kwargs.get('livox_timestamp')
        self.data = array.array('B', kwargs.get('data', []))

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.get_fields_and_field_types().keys(), self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    if self._check_fields:
                        assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.data_type != other.data_type:
            return False
        if self.dot_num != other.dot_num:
            return False
        if self.time_interval != other.time_interval:
            return False
        if self.udp_cnt != other.udp_cnt:
            return False
        if self.frame_cnt != other.frame_cnt:
            return False
        if self.time_type != other.time_type:
            return False
        if any(self.livox_timestamp != other.livox_timestamp):
            return False
        if self.data != other.data:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def data_type(self):
        """Message field 'data_type'."""
        return self._data_type

    @data_type.setter
    def data_type(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'data_type' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'data_type' field must be an unsigned integer in [0, 255]"
        self._data_type = value

    @builtins.property
    def dot_num(self):
        """Message field 'dot_num'."""
        return self._dot_num

    @dot_num.setter
    def dot_num(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'dot_num' field must be of type 'int'"
            assert value >= 0 and value < 65536, \
                "The 'dot_num' field must be an unsigned integer in [0, 65535]"
        self._dot_num = value

    @builtins.property
    def time_interval(self):
        """Message field 'time_interval'."""
        return self._time_interval

    @time_interval.setter
    def time_interval(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'time_interval' field must be of type 'int'"
            assert value >= 0 and value < 65536, \
                "The 'time_interval' field must be an unsigned integer in [0, 65535]"
        self._time_interval = value

    @builtins.property
    def udp_cnt(self):
        """Message field 'udp_cnt'."""
        return self._udp_cnt

    @udp_cnt.setter
    def udp_cnt(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'udp_cnt' field must be of type 'int'"
            assert value >= 0 and value < 65536, \
                "The 'udp_cnt' field must be an unsigned integer in [0, 65535]"
        self._udp_cnt = value

    @builtins.property
    def frame_cnt(self):
        """Message field 'frame_cnt'."""
        return self._frame_cnt

    @frame_cnt.setter
    def frame_cnt(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'frame_cnt' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'frame_cnt' field must be an unsigned integer in [0, 255]"
        self._frame_cnt = value

    @builtins.property
    def time_type(self):
        """Message field 'time_type'."""
        return self._time_type

    @time_type.setter
    def time_type(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'time_type' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'time_type' field must be an unsigned integer in [0, 255]"
        self._time_type = value

    @builtins.property
    def livox_timestamp(self):
        """Message field 'livox_timestamp'."""
        return self._livox_timestamp

    @livox_timestamp.setter
    def livox_timestamp(self, value):
        if self._check_fields:
            if isinstance(value, numpy.ndarray):
                assert value.dtype == numpy.uint8, \
                    "The 'livox_timestamp' numpy.ndarray() must have the dtype of 'numpy.uint8'"
                assert value.size == 8, \
                    "The 'livox_timestamp' numpy.ndarray() must have a size of 8"
                self._livox_timestamp = value
                return
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 8 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 256 for val in value)), \
                "The 'livox_timestamp' field must be a set or sequence with length 8 and each value of type 'int' and each unsigned integer in [0, 255]"
        self._livox_timestamp = numpy.array(value, dtype=numpy.uint8)

    @builtins.property
    def data(self):
        """Message field 'data'."""
        return self._data

    @data.setter
    def data(self, value):
        if self._check_fields:
            if isinstance(value, array.array):
                assert value.typecode == 'B', \
                    "The 'data' array.array() must have the type code of 'B'"
                self._data = value
                return
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 256 for val in value)), \
                "The 'data' field must be a set or sequence and each value of type 'int' and each unsigned integer in [0, 255]"
        self._data = array.array('B', value)
