open Belt;
open Styp;

let simpleNull = true;
let simpleEmptyArray = true;
let skip100Percent = true;
let skip0OutOf0 = true;
let percent = false;

let pToString = (p, ~ctx) => {
  let p0 =
    switch (ctx) {
    | None => p
    | Some(p0) => p0
    };
  if (percent && p0 != 0) {
    let perc = float_of_int(p) /. float_of_int(p0);
    string_of_float(perc);
  } else {
    string_of_int(p) ++ "/" ++ string_of_int(p0);
  };
};

let pToJson = (p, ~ctx) => p |. pToString(~ctx) |. Js.Json.string;

let is100Percent = (p, ~ctx) => ctx == None || ctx == Some(p);

let is0OutOf0 = (p, ~ctx) => p == 0 && (ctx == None || ctx == Some(0));

let addStats = (jsonT, ~o, ~p, ~skipP, ~ctx) => {
  let jsonStats = {
    let pString = p |. pToString(~ctx);
    let oString = o == Opt ? "?" : "";
    oString ++ pString |. Js.Json.string;
  };
  switch (Js.Json.classify(jsonT)) {
  | JSONObject(dict) =>
    dict |. Js.Dict.set("_stats", jsonStats);
    dict |. Js.Json.object_;
  | JSONArray(arr) =>
    let jsonNew =
      [|("_stats", jsonStats)|] |. Js.Dict.fromArray |. Js.Json.object_;
    arr |. Js.Array.concat([|jsonNew|]) |. Js.Json.array;
  | _ =>
    if (skipP && o == NotOpt) {
      jsonT;
    } else {
      let dict = Js.Dict.empty();
      dict |. Js.Dict.set("typ", jsonT);
      dict |. Js.Dict.set("_stats", jsonStats);
      dict |. Js.Json.object_;
    }
  };
};
let rec toJson = (styp: styp, ~ctx: option(p)) : Js.Json.t =>
  if (simpleNull && styp |. stypIsNull) {
    Js.Json.null;
  } else {
    let skipP =
      skip100Percent
      && styp.p
      |. is100Percent(~ctx)
      || skip0OutOf0
      && styp.p
      |. is0OutOf0(~ctx);
    let jsonT = styp.t |. toJsonT(~ctx=Some(styp.p));
    switch (skipP, styp.o) {
    | (true, NotOpt) => jsonT
    | _ => jsonT |. addStats(~o=styp.o, ~p=styp.p, ~skipP, ~ctx)
    };
  }
and toJsonT = (t: t, ~ctx: option(p)) : Js.Json.t =>
  switch (t) {
  | Same => Js.Json.string("same")
  | Number => Js.Json.string("number")
  | String => Js.Json.string("string")
  | Boolean => Js.Json.string("boolean")
  | Object(d) =>
    let doEntry = ((lbl, styp)) => (lbl, styp |. toJson(~ctx));
    Js.Dict.entries(d)
    |. Array.map(doEntry)
    |. Js.Dict.fromArray
    |. Js.Json.object_;
  | Array(styp) when simpleEmptyArray && stypIsSame(styp) =>
    [||] |. Js.Json.array
  | Array(styp) => [|styp |. toJson(~ctx)|] |. Js.Json.array
  };
let styp = styp => styp |. toJson(~ctx=None) |. Js.Json.stringify;