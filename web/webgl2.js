var GL_DEPTH_COMPONENT = 0x1902
var GL_DEPTH_STENCIL = 0x84F9
var HALF_FLOAT_OES = 0x8D61

// webgl1 extensions natively supported by webgl2
var gl2Extensions = {
  'WEBGL_depth_texture': {
    'UNSIGNED_INT_24_8_WEBGL': 0x84FA
  },
  'OES_element_index_uint': {},
  'OES_texture_float': {},
  'OES_texture_float_linear': {},
  'OES_texture_half_float': {
    'HALF_FLOAT_OES': 0x8D61
  },
  'OES_texture_half_float_linear': {},
  'EXT_color_buffer_float': {},
  'OES_standard_derivatives': {},
  'EXT_frag_depth': {},
  'EXT_blend_minmax': {
    'MIN_EXT': 0x8007,
    'MAX_EXT': 0x8008
  },
  'EXT_shader_texture_lod': {}
}

module.exports = {
  // webgl1 extensions natively supported by webgl2
  // this is called when initializing regl context
  gl2: function (gl, extensions) {
    gl[this.versionProperty] = 2
    for (var p in gl2Extensions) {
      extensions[p.toLowerCase()] = gl2Extensions[p]
    }

    // to support float and half-float textures
    gl.getExtension('EXT_color_buffer_float');

    // mocks of draw buffers's functions
    extensions['webgl_draw_buffers'] = {
      drawBuffersWEBGL: function () {
        return gl.drawBuffers.apply(gl, arguments)
      }
    }

    // mocks of vao extension
    extensions['oes_vertex_array_object'] = {
      'VERTEX_ARRAY_BINDING_OES':   0x85B5,
      'createVertexArrayOES': function () {
        return gl.createVertexArray()
      },
      'deleteVertexArrayOES': function () {
        return gl.deleteVertexArray.apply(gl, arguments)
      },
      'isVertexArrayOES': function () {
        return gl.isVertexArray.apply(gl, arguments)
      },
      'bindVertexArrayOES': function () {
        return gl.bindVertexArray.apply(gl, arguments)
      }
    }

    // mocks of instancing extension
    extensions['angle_instanced_arrays'] = {
      'VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE': 0x88FE,
      'drawArraysInstancedANGLE': function () {
        return gl.drawArraysInstanced.apply(gl, arguments)
      },
      'drawElementsInstancedANGLE': function () {
        return gl.drawElementsInstanced.apply(gl, arguments)
      },
      'vertexAttribDivisorANGLE': function () {
        return gl.vertexAttribDivisor.apply(gl, arguments)
      }
    }
  },

  versionProperty: '___regl_gl_version___',

  // texture internal format to update on the fly
  getInternalFormat: function (gl, format, type) {
    if (gl[this.versionProperty] !== 2) {
      return format
    }
    // webgl2 texture formats
    // reference:
    // https://webgl2fundamentals.org/webgl/lessons/webgl-data-textures.html
    if (format === GL_DEPTH_COMPONENT) {
      // gl.DEPTH_COMPONENT24
      return 0x81A6
    } else if (format === GL_DEPTH_STENCIL) {
      // gl.DEPTH24_STENCIL8
      return 0x88F0
    } else if (type === HALF_FLOAT_OES && format === gl.RGBA) {
      // gl.RGBA16F
      return 0x881A
    } else if (type === HALF_FLOAT_OES && format === gl.RGB) {
      // gl.RGB16F
      return 0x881B
    } else if (type === gl.FLOAT && format === gl.RGBA) {
      // gl.RGBA32F
      return 0x8814
    } else if (type === gl.FLOAT && format === gl.RGB) {
      // gl.RGB32F
      return 0x8815
    }
    return format
  },

  // texture type to update on the fly
  getTextureType: function (gl, type) {
    if (gl[this.versionProperty] !== 2) {
      return type
    }
    if (type === HALF_FLOAT_OES) {
      return gl.HALF_FLOAT
    }
    return type
  },
}