open Belt;
open Styp;

let simpleNull = true;
let simpleEmptyArray = true;
let skip100Percent = true;
let skip0OutOf0 = true;

let pToString = (p, ~ctx) => P.toString(p) ++ "/" ++ P.toString(ctx);

let pToJson = (p, ~ctx) => p |. pToString(~ctx) |. Js.Json.string;

let is100Percent = (p, ~ctx) => ctx == P.zero || ctx == p;

let is0OutOf0 = (p, ~ctx) => p == P.zero && ctx == P.zero;

let addStats = (jsonT, ~o, ~p, ~skipP, ~ctx) => {
  let jsonStats = {
    let pString = p |. pToString(~ctx);
    let oString =
      switch (o) {
      | NotOpt => ""
      | Opt(p1) => "?" ++ P.toString(p1)
      };

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
let rec toJsonStyp = (styp: styp, ~ctx: p) : Js.Json.t =>
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
    let jsonT = styp.typ |. toJsonTyp(~ctx=styp.p);
    switch (skipP, styp.o) {
    | (true, NotOpt) => jsonT
    | _ => jsonT |. addStats(~o=styp.o, ~p=styp.p, ~skipP, ~ctx)
    };
  }
and toJsonTyp = (typ: typ, ~ctx: p) : Js.Json.t =>
  switch (typ) {
  | Empty(_) => Js.Json.string("empty")
  | Number(_)
  | String(_)
  | Boolean(_) => typ |. constToString |. Js.Json.string
  | Object(d) =>
    let doEntry = ((lbl, styp)) => (lbl, styp |. toJsonStyp(~ctx));
    Js.Dict.entries(d)
    |. Array.map(doEntry)
    |. Js.Dict.fromArray
    |. Js.Json.object_;
  | Array(styp) when simpleEmptyArray && stypIsEmpty(styp) =>
    [||] |. Js.Json.array
  | Array(styp) => [|styp |. toJsonStyp(~ctx)|] |. Js.Json.array
  | Union(styps) =>
    styps |. List.map(toJsonStyp(~ctx)) |. List.toArray |. Js.Json.array
  | Diff(typ, _, _) => typ |. toJsonTyp(~ctx)
  };
let styp = styp => styp |. toJsonStyp(~ctx=P.zero) |. Js.Json.stringify;