'use strict';

var Victor = (function() {
  var v = {};

  v.layers = [];

  v.options = {
    shape:        'ellipse',
    origin:       {x: 500, y: 100},
    size:         {x: 400, y: 2},
    algorithm:    'old',
    scatter:      10,
    rotation:     {deg: Math.random() * 360, delta: 10},
    color:        '#FFFFFF',
    brightness:   0,
    minsize:      -5,
    maxsize:      5,
    layers:       5,
    iterations:   100
  };

  v.randomRange = function(min, max) {
    return Math.random() * (max - min) + min;
  };

  v.shapes = {
    'ellipse': function(options) {
      var shapes = v.paper.set();
      shapes.push(v.paper.ellipse(options.origin.x, options.origin.y, options.size.x, options.size.y));
      return shapes;
    },
    'rtriangle': function(options) {
      var a = {
        x: v.randomRange(Math.cos(150 * Math.PI/180).toFixed(2) * options.size.x, Math.sin(150 * Math.PI/180).toFixed(2) * options.size.x),
        y: v.randomRange(Math.cos(30 * Math.PI/180).toFixed(2) * options.size.x, Math.sin(30 * Math.PI/180).toFixed(2) * options.size.x)
      };
      var b = {
        x: v.randomRange(Math.cos(30 * Math.PI/180).toFixed(2) * options.size.x, Math.sin(30 * Math.PI/180).toFixed(2) * options.size.x),
        y: v.randomRange(Math.cos(270 * Math.PI/180).toFixed(2) * options.size.x, Math.sin(270 * Math.PI/180).toFixed(2) * options.size.x)
      };
      var c = {
        x: v.randomRange(Math.cos(270 * Math.PI/180).toFixed(2) * options.size.x, Math.sin(270 * Math.PI/180).toFixed(2) * options.size.x),
        y: v.randomRange(Math.cos(150 * Math.PI/180).toFixed(2) * options.size.x, Math.sin(150 * Math.PI/180).toFixed(2) * options.size.x)
      };
      var shapes = v.paper.set();
      shapes.push(v.paper.path('M' + a.x + ',' + a.y + 'L' + b.x + ',' + b.y + 'L' + c.x + ',' + c.y));
      return shapes;
    },
    'triangle': function(options) {
      var a = {
        x: Math.cos(30 * Math.PI/180).toFixed(2) * options.size.x,
        y: Math.sin(30 * Math.PI/180).toFixed(2) * options.size.x
      };
      var b = {
        x: Math.cos(150 * Math.PI/180).toFixed(2) * options.size.x,
        y: Math.sin(150 * Math.PI/180).toFixed(2) * options.size.x
      };
      var c = {
        x: Math.cos(270 * Math.PI/180).toFixed(2) * options.size.x,
        y: Math.sin(270 * Math.PI/180).toFixed(2) * options.size.x
      };
      var shapes = v.paper.set();
      shapes.push(v.paper.path('M' + a.x + ',' + a.y + 'L' + b.x + ',' + b.y + 'L' + c.x + ',' + c.y));
      return shapes;
    },
    'splatter': function(options) {
      var o = {};
      jQuery.extend(o, options);
      o.shape = 'ellipse';
      o.algorithm = 'old';
      o.size = {x: 100, y: 0.2};
      o.origin = {x: 0, y: 0};
      o.scatter = 100;
      o.iterations = 2;
      o.layers = 2;
      var set = v.paper.set();
      set.push(v.draw(o));
      set.push(v.shapes.rtriangle(options));
      return set;
    }
  };

  v.step = function(el, time) {
    var seed1 = Math.random();
    var posx, posy, deg;
    if (seed1 < 0.5) {
      posx = -Math.random() * 5;
      posy = -Math.random() * 5;
      deg = -Math.random() * 5;
    }
    else {
      posx = Math.random() * 5;
      posy = Math.random() * 5;
      deg = Math.random() * 5;
    }
    el.animate({transform: '...t' + posx + ',' + posy + 'r' + deg + ',' + el.attrs.cx + ',' + el.attrs.cy}, time, '>');
  };

  v.draw = function(options) {
    var posx = options.origin.x;
    var posy = options.origin.y;
    var deg = options.rotation.deg;
    var col = Raphael.getRGB(options.color);
    col.r += options.brightness;
    col.g += options.brightness;
    col.b += options.brightness;
    var layers = this.paper.set();
    var r, g, b, color, sizex, sizey, shapes, seed1, seed2, offsetx, offsety, deltax, deltay;

    for (var j = 0; j < options.layers; j++) {
      // Calculate next layer's color
      r = col.r - (255 - col.r) / (options.layers - 1) * j;
      g = col.g - (255 - col.g) / (options.layers - 1) * j;
      b = col.b - (255 - col.b) / (options.layers - 1) * j;
      if (r < 0) {
        r = 0;
      }
      if (g < 0) {
        g = 0;
      }
      if (b < 0) {
        b = 0;
      }
      color = Raphael.rgb(r, g, b);
      // Calculate next shape's base size (reducing)
      sizex = (1 - (j * 1 / options.layers));
      sizey = (1 - (j * 1 / options.layers));
      for (var i = 0; i < options.iterations; i++) {
        // Draw new shapes
        shapes = this.shapes[options.shape](options);
        // Random seeds
        seed1 = Math.random();
        seed2 = Math.random();

        // Calculate random variations
        offsetx = offsety = (j * options.scatter / options.layers);
        if (options.algorithm === 'old') {
          offsetx = offsety = (i * options.scatter / options.iterations);
        }

        deltax = deltay = (i * Math.random() / 10);
        if (seed1 < 0.5) {
          offsetx = -offsetx;
          deltax = -deltax;
        }
        if (seed2 < 0.5) {
          offsety = -offsety;
          deltay = -deltay;
        }

        if (options.algorithm === 'old') {
          posx += offsetx;
          posy += offsety;
          deg = seed1 > 0.5 ? deg + Math.random() * options.rotation.delta : deg - Math.random() * options.rotation.delta;
        }
        else {
          posx = options.origin.x + offsetx;
          posy = options.origin.y + offsety;
          deg = seed1 > 0.5 ? options.rotation.deg + Math.random() * options.rotation.delta : options.rotation.deg - Math.random() * options.rotation.delta;
        }

        // New size
        sizex += deltax;
        sizex = options.maxsize && sizex > options.maxsize ? options.maxsize : sizex;
        sizex = options.minsize && sizex < options.minsize ? options.minsize : sizex;
        sizey += deltay;
        sizey = options.maxsize && sizey > options.maxsize ? options.maxsize : sizey;
        sizey = options.minsize && sizey < options.minsize ? options.minsize : sizey;

        shapes.forEach(function(shape) {
          if (options.algorithm === 'old') {
            shape.transform('...T' + posx + ',' + posy + 'S' + sizex + ',' + sizey + 'r' + deg + ',' + posx + ',' + posy);
          }
          else {
            shape.transform('...t' + posx + ',' + posy + 'S' + sizex + ',' + sizey + ',' + options.origin.x + ',' + options.origin.y + 'r' + deg + ',' + options.origin.x + ',' + options.origin.y);
          }
          shape.attr({stroke: 0, fill: color});
          layers.push(shape);
        });
      }
    }
    return layers;
  };

  v.animate = function(set, time) {
    set.forEach(function(el) {
      v.step(el, time);
    });
  };

  v.init = function(id, width, height) {
    this.id = id;
    this.paper = new Raphael(id, width, height);
    return this;
  };

  v.go = function(options) {
    jQuery.extend(this.options, options);
    var l = this.draw(this.options);
    this.layers.push(l);
    return l;
  };

  v.svg = function() {
    // loop through the 'paper' variable from Raphael JS and build up the JSON object describing all images and paths within it.
    buildJSON = function() {
      var svgdata = [];
      svgdata.push({
        width: $('#' + v.id).width(),
        height: $('#' + v.id).height()
      });

      $.each(v.paper, function(i, o) {
        var node = o;
        var object;
        if (node && node.type) {
          switch(node.type) {
            case 'image':
              object = {
                type: node.type,
                width: node.attrs.width,
                height: node.attrs.height,
                x: node.attrs.x,
                y: node.attrs.y,
                src: node.attrs.src,
                transform: node.transformations ? node.transformations.join(' ') : ''
              };
              break;
            case 'ellipse':
              object = {
                type: node.type,
                rx: node.attrs.rx,
                ry: node.attrs.ry,
                cx: node.attrs.cx,
                cy: node.attrs.cy,
                stroke: node.attrs.stroke === 0 ? 'none': node.attrs.stroke,
                'stroke-width': node.attrs['stroke-width'],
                fill: node.attrs.fill
              };
              break;
            case 'rect':
              object = {
                type: node.type,
                x: node.attrs.x,
                y: node.attrs.y,
                width: node.attrs.width,
                height: node.attrs.height,
                stroke: node.attrs.stroke === 0 ? 'none': node.attrs.stroke,
                'stroke-width': node.attrs['stroke-width'],
                fill: node.attrs.fill
              };
              break;
            case 'text':
              object = {
                type: node.type,
                font: node.attrs.font,
                'font-family': node.attrs['font-family'],
                'font-size': node.attrs['font-size'],
                stroke: node.attrs.stroke === 0 ? 'none': node.attrs.stroke,
                fill: node.attrs.fill === 0 ? 'none' : node.attrs.fill,
                'stroke-width': node.attrs['stroke-width'],
                x: node.attrs.x,
                y: node.attrs.y,
                text: node.attrs.text,
                'text-anchor': node.attrs['text-anchor']
              };
              break;

            case 'path':
              var path = '';

              if(node.attrs.path.constructor !== Array){
                path += node.attrs.path;
              }
              else{
                $.each(node.attrs.path, function(i, group) {
                  $.each(group,
                    function(index, value) {
                      if (index < 1) {
                          path += value;
                      } else {
                        if (index === (group.length - 1)) {
                          path += value;
                        } else {
                         path += value + ',';
                        }
                      }
                    });
                });
              }

              object = {
                type: node.type,
                fill: node.attrs.fill,
                opacity: node.attrs.opacity,
                translation: node.attrs.translation,
                scale: node.attrs.scale,
                path: path,
                stroke: node.attrs.stroke === 0 ? 'none': node.attrs.stroke,
                'stroke-width': node.attrs['stroke-width'],
                transform: node.transformations ? node.transformations.join(' ') : ''
              };
          }
          if (object) {
            svgdata.push(object);
          }
        }
      });
      return JSON.stringify(svgdata);
    };

    var svg = '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1" width="' + $('#' + this.id).width() + '" height="' + $('#' + this.id).height() + '" xml:space="preserve"><desc>Created with Raphael</desc><defs></defs>\n';
    this.paper.forEach(function (node) {
      if (node && node.type) {
        switch (node.type) {
          case 'ellipse':
            var object = {
              type: node.type,
              rx: node.attrs.rx,
              ry: node.attrs.ry,
              cx: node.attrs.cx,
              cy: node.attrs.cy,
              stroke: node.attrs.stroke === 0 ? 'none': node.attrs.stroke,
              fill: node.attrs.fill,
              transform: 'matrix(' + node.matrix.a + ', ' + node.matrix.b + ', ' + node.matrix.c + ', ' + node.matrix.d + ', ' + node.matrix.e + ', ' + node.matrix.f + ')'
            };
            svg += '<ellipse rx="' + object.rx + '" ry="' + object.ry + '" cx="' + object.cx + '" cy="' + object.cy + '" style="stroke: ' + object.stroke + '; fill: ' + object.fill + ';" transform="' + object.transform + '"></ellipse>\n';
            break;
        }
      }
    });
    svg += '</svg>';

    return new Blob([svg], {type: 'application/octet-binary'});
  };

  return v;
}());
